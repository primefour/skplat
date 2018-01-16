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
/*
#include"RawFile.h"
#include"BufferUtils.h"
*/
#define LOG_EXT "log"
#define LOG_PREFIX "skplat"

#ifndef LOG_TAG 
#define LOG_TAG "SKPLATLOG"
#endif

class RawFile;
//class Mutex;
class BufferUtils;

/*
 * App file log implement writing log into file and console 
 * we will add gzip compress function for file log lately
 */
class AppFileLog{
    public:
        AppFileLog();
        ~AppFileLog();
        //set dir for file log
        void setDir(const char* dir);
        void close(); 
        void flush();
        void setLevel(int level);
        //write msg to log file
        void write(const char* logMsg,int len);
        //write buffer content to log file
        void write(const BufferUtils &buffer);
    private:
        RawFile* openFile();
        std::string mLogDir;
        Mutex mMutex;
        RawFile *mFile;
        BufferUtils *mBuffer;
        time_t mOpenFileTime;
};
#endif
