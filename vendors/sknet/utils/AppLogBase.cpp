#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include "AppFileLog.h"
#include "AppLogBase.h"
#include "FileUtils.h"

#define MAX_LINE_LEN (2048)
static const char* levelStrings[] = {
    "V",
    "D",  // debug
    "I",  // info
    "W",  // warn
    "E",  // error
    "F"  // fatal
};

struct AppLogBase{
    AppLogBase(){
        mLevel = LogLevelAll;
        mConsole = true;
    }

    //check log level
    bool isLog(int level){ return level > mLevel; }

    bool isConsole(){ return mConsole;}

    void setLevel(int level){
        if(level < LogLevelAll){
            mLevel = LogLevelAll; 
        }else if(level > LogLevelNone){
            mLevel = LogLevelNone;
        }else{
            mLevel = level;
        }
    }

    //getpid wrap
    static int getPid(){ return ::getpid();} 
    static int getTid(){ return ::gettid(); }
    static int getMainPid(){ return ::getpid(); }

    //get thread's info for log entry
    static void initEntry(LogEntry* logInfo) {
        //fetch time
        if (0 == logInfo->tv.tv_sec) {
            gettimeofday(&(logInfo->tv),NULL);
        }
        //fetch pid info
        if(logInfo != NULL && logInfo->pid == -1){
            logInfo->pid = getPid();
            logInfo->tid = getTid();
            logInfo->mainPid = getMainPid();
        }
    }

    //format log as android format -v threadtime format for writing to file
    static int skFormatInfo(const LogEntry* logInfo,char *dataBuff,int buffLen) {
        if(dataBuff == NULL){
            return 0 ;
        }
        char *logData = dataBuff;
        std::string fileName = FileUtils::extractFileName(logInfo->fileName);
        char timeString [64] = {0};
        //format time
        time_t sec = logInfo->tv.tv_sec;
        tm tm = *localtime((const time_t*)&sec);
        snprintf(timeString, sizeof(timeString) -1, "%d-%02d-%02d %+.1f %02d:%02d:%02d.%.3ld", 
                1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday,
                tm.tm_gmtoff / 3600.0, tm.tm_hour, tm.tm_min, tm.tm_sec, logInfo->tv.tv_usec / 1000);
        //log format like android time->mainpid->tid->loglevel->log tag->file name->function name -> line ->msg
        int ret = snprintf(logData,buffLen,"%s %d %d %s %s: [%s %s %d] ",timeString,logInfo->mainPid,logInfo->tid,
                levelStrings[logInfo->level], logInfo->tag == NULL?"sklog":logInfo->tag,
                fileName.c_str(), logInfo->funcName, logInfo->line);
        return ret;
    }

    void setFileLogDir(const char *logPath){
        mFileLog.setDir(logPath);
    }

    void writeFile(const char *logMsg,int len){
        mFileLog.write(logMsg,len);
    }

    void writeFile(BufferUtils& buffer){
        mFileLog.write(buffer);
    }

    void flush(){
        mFileLog.flush();
    }

    int mLevel;
    bool mConsole;
    AppFileLog mFileLog;
};

static AppLogBase gAppLogger;

static void printConsole(LogEntry *logInfo,const char* logMsg){
	char printString[2048] = {0};
    if (logInfo != NULL) {
        std::string fileName= FileUtils::extractFileName(logInfo->fileName);
        snprintf(printString, sizeof(printString) -1, "[%s, %s, %d]:%s",fileName.c_str(),
                logInfo->funcName,logInfo->line, logMsg?logMsg:"logMsg is NULL");
    } else {
    	snprintf(printString, sizeof(printString)-1 , "%s", logMsg?logMsg:"logMsg is NULL");
    }
#ifdef ANDROID
#include <android/log.h>
    if (logInfo != NULL) {
        __android_log_print(logInfo->level+2, logInfo->tag ?logInfo->tag:LOG_TAG,"%s",(const char*)printString);
    }else{
        __android_log_print(ANDROID_LOG_WARN,LOG_TAG,"%s",printString);
    }
#else
#ifdef APPLE
    

#else
    printf("%s:%s ",LOG_TAG,printString);
    printf("\n");
#endif //apple
#endif //android
}


void skLogSetDir(const char *dir){
    gAppLogger.setFileLogDir(dir);
}

void skLogSetLevel(int level){
    gAppLogger.setLevel(level);
}

bool skLogCheckLevel(int level){
    return gAppLogger.isLog(level);
}

void skLogFlush(){
    gAppLogger.flush();
}

void skLogPrint(LogEntry *logInfo,const char* format, ...){
    if(logInfo == NULL || format == NULL){
        return;
    }
    char tmpBuff[4096] = {0};
    AppLogBase::initEntry(logInfo);
    int ret = AppLogBase::skFormatInfo(logInfo,tmpBuff,sizeof(tmpBuff) -1);
    if(ret < 0){
        return ;
    }
    char *tmpPtr = tmpBuff + ret;
    int left = sizeof(tmpBuff) - ret -1;

	va_list valist;
	va_start(valist,format);
    ret = vsnprintf(tmpPtr,left,format,valist);
	va_end(valist);

    if(ret < 0){
        return;
    }

    int size = tmpPtr - tmpBuff + ret;

    if(gAppLogger.isConsole()){
        printConsole(logInfo,tmpPtr);
    }
    *(tmpBuff+size) = '\n';
    gAppLogger.writeFile(tmpBuff,size +1);
}

static const char *AssertLogMsg = "ASSERT ERROR:";

void skLogAssert(LogEntry *logInfo,const char *condition,const char* format, ...){
    if(logInfo == NULL || condition == NULL){
        return ;
    }
    char tmpBuff[4096] = {0};
    AppLogBase::initEntry(logInfo);
    int ret = AppLogBase::skFormatInfo(logInfo,tmpBuff,sizeof(tmpBuff) -1);
    if(ret < 0){
        return ;
    }
    char *tmpPtr = tmpBuff + ret;
    char *msg = tmpPtr;
    strcpy(tmpPtr,AssertLogMsg);
    tmpPtr += strlen(AssertLogMsg);
    strcpy(tmpPtr,condition);
    tmpPtr += strlen(condition);

    int left = sizeof(tmpBuff) + tmpBuff - tmpPtr -1;
    ret = 0;
    if(format != NULL){
        va_list valist;
        va_start(valist,format);
        ret = vsnprintf(tmpPtr,left,format,valist);
        va_end(valist);
    }

    int size = tmpPtr - tmpBuff + ret;

    if(gAppLogger.isConsole()){
        printConsole(logInfo,msg);
    }
    *(tmpBuff+size) = '\n';
    gAppLogger.writeFile(tmpBuff,size +1);
    assert(false);
}

