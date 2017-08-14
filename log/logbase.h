#ifndef __LOG_UTIL_H__ 
#define __LOG_UTIL_H__ 

#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LINE_LEN (2*1024)

typedef enum {
    kLevelAll = 0,
    kLevelVerbose = 0,
    kLevelDebug,    // Detailed information on the flow through the system.
    kLevelInfo,     // Interesting runtime events (startup/shutdown), should be conservative and keep to a minimum.
    kLevelWarn,     // Other runtime situations that are undesirable or unexpected, but not necessarily "wrong".
    kLevelError,    // Other runtime errors or unexpected conditions.
    kLevelFatal,    // Severe errors that cause premature termination.
    kLevelNone,     // Special level used to disable all log messages.
} LogLevel;

typedef struct LogInfo_t {
    LogLevel level;
    const char* tag;
    const char* filename;
    const char* func_name;
    int line;
    struct timeval timeval;
    int pid;
    int tid;
    int maintid;
} LogInfo;

//interface for platform implements
class LogBaseIntf{
    public:
        virtual void SetParameters(const char *dir,const char *prefix)=0;
        virtual void FilePrint(const char *_log) = 0;
        virtual void FileFlush() = 0;
        virtual void Close() = 0;
        virtual void DumpBuffer(const void *buffer,size_t _len) =0;
        virtual intmax_t LogGetPid() = 0;
        virtual intmax_t LogGetTid() = 0;
        virtual intmax_t LogGetMainTid() = 0;
};

//interface for platform call
LogLevel log_get_level();
void log_set_level(LogLevel _level);
int  log_for(LogLevel _level);
void log_set_enable(bool enable);
void log_flush();
void log_file_init(const char* _dir, const char* _nameprefix);
void log_file_close();
void log_set_console_enable(bool enable);

#ifdef __GNUC__
__attribute__((__format__(printf, 3, 4)))
#endif
void        log_assertp(const LogInfo* _info, const char* _expression, const char* _format, ...);
void        log_assert(const LogInfo* _info, const char* _expression, const char* _log);
#ifdef __GNUC__
__attribute__((__format__(printf, 2, 0)))
#endif
void        log_vprint(const LogInfo* _info, const char* _format, va_list _list);
#ifdef __GNUC__
__attribute__((__format__(printf, 2, 3)))
#endif
void        log_print(const LogInfo* _info, const char* _format, ...);
void        log_write(const LogInfo* _info, const char* _log);
#ifdef __cplusplus
}
#endif

#endif
