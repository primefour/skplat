#ifndef __APP_FILE_LOG_H__
#define __APP_FILE_LOG_H__
#include<sys/types.h>
#include<unistd.h> 
#include<stdio.h>
#include<time.h>
#include<string>
#include<stdarg.h>
#if defined(HAVE_PTHREADS)
# include <unistd.h>
# include <pthread.h>
# include <sched.h>
# include <sys/resource.h>
# include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif
#include"Mutex.h"
#include"RawFile.h"
#include"BufferUtils.h"
#define LOG_EXT "log"
#define LOG_PREFIX "skplat"
#define LOG_TAG "SKPLATLOG"


enum{
    LogLevelAll = 0,
    LogLevelVerbose = 0,
    LogLevelDebug,    // Detailed information on the flow through the system.
    LogLevelInfo,     // Interesting runtime events (startup/shutdown), should be conservative and keep to a minimum.
    LogLevelWarn,     // Other runtime situations that are undesirable or unexpected, but not necessarily "wrong".
    LogLevelError,    // Other runtime errors or unexpected conditions.
    LogLevelFatal,    // Severe errors that cause premature termination.
    LogLevelNone,     // Special level used to disable all log messages.
};

struct LogEntry {
    LogEntry(){
        tag = NULL;
        fileName = NULL;
        funcName = NULL;
        line = 0;
        level = 0;
        pid = -1;
        tid = -1;
        mainPid = -1;
    }
    int level;
    const char *tag;
    const char *fileName;
    const char *funcName;
    int line;
    struct timeval tv;
    int pid;
    int tid;
    int mainPid;
};

class AppFileLog{
    public:
        AppFileLog();
        ~AppFileLog();
        void setDir(const char* dir);
        void write(const char* logMsg);
        void write(const BufferUtils &buffer);

        void close(); 
        void flush();
        void setLevel(int level);
        void printLog(LogEntry *logInfo,const char *format,...);
        void assertLog(LogEntry *logInfo,const char *condition,const char *format,...);
        inline bool isLog(int level){ return level > mLevel; }
    private:
        RawFile* openFile();
        inline int getPid(){ return ::getpid();} 
        inline int getTid(){ return ::gettid(); }
        inline int getMainPid(){ return ::getpid(); }
        void initEntry(LogEntry* logInfo);
        void formatLog(const LogEntry* logInfo, const char* logMsg, BufferUtils *buffer);
        std::string mLogDir;
        Mutex mMutex;
        RawFile *mFile;
        BufferUtils *mBuffer;
        time_t mOpenFileTime;
        bool mConsole;
        int mLevel;
};
#endif
