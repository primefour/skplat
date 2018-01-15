#include <fcntl.h>
#include <stdio.h>
#include <inttypes.h>
#include <ctype.h>
#include <assert.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>  
#include <stdlib.h>  
#include <stdarg.h>
#include <sys/time.h>
#include "AppFileLog.h"
#include "FileUtils.h"
#include "RawFile.h"


void printConsole(LogEntry *logInfo,const char* logMsg){
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

#define MAX_LINE_LEN (2048)

static const char* levelStrings[] = {
    "V",
    "D",  // debug
    "I",  // info
    "W",  // warn
    "E",  // error
    "F"  // fatal
};


AppFileLog::AppFileLog(){
    mOpenFileTime = 0;
    mBuffer = new BufferUtils(MAX_LINE_LEN);
}

AppFileLog::~AppFileLog(){
    if(mFile != NULL){
        mFile->flush();
        delete mFile;
    }
}

void AppFileLog::write(const char *logMsg) {
    AutoMutex _l(mMutex);
    if(mFile != NULL){
        mFile->write(logMsg,strlen(logMsg));
    }
}

void AppFileLog::write(const BufferUtils &buffer) {
    AutoMutex _l(mMutex);
    if(mFile != NULL){
        mFile->write(buffer.data(),buffer.size());
    }
}

void AppFileLog::setDir(const char *dir){
    int ret = 0;
    AutoMutex _l(mMutex);
    if(dir == NULL){
        return;
    }

    if(FileUtils::isDir(dir)){
        ret = FileUtils::makeDir(dir);
        if(!ret){
            return;
        }

    }
    if(!mLogDir.empty() && dir == mLogDir){
        return;
    }
    mLogDir = dir;
}

RawFile* AppFileLog::openFile() {
    AutoMutex _l(mMutex);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (NULL != mFile) {
        time_t sec = tv.tv_sec;
        tm tcur = *localtime((const time_t*)&sec);
        tm filetm = *localtime(&mOpenFileTime);
        //one log file per day
        if (filetm.tm_year == tcur.tm_year && filetm.tm_mon == tcur.tm_mon && 
                filetm.tm_mday == tcur.tm_mday){
            return mFile;
        }
    }

    mOpenFileTime = tv.tv_sec;
    std::string filePath = FileUtils::makeFileNameByTime(mLogDir.c_str(),LOG_PREFIX,LOG_EXT);

    if(mFile != NULL){
        delete mFile ;
        mFile = NULL;
    }

    mFile = new RawFile(filePath.c_str());
    int ret = mFile->open();
    if(ret != OK){
        delete mFile;
        mFile = NULL;
        return NULL;
    }
    return mFile;;
}

void AppFileLog::setLevel(int level){
    if(level < LogLevelAll){
        mLevel = LogLevelAll; 
    }else if(level > LogLevelNone){
        mLevel = LogLevelNone;
    }else{
        mLevel = level;
    }
}

void AppFileLog::initEntry(LogEntry* logInfo) {
    //fetch time
    if (0 == logInfo->tv.tv_sec) {
        gettimeofday(&(logInfo->tv),NULL);
    }
    //fetch pid info
    if(logInfo != NULL && logInfo->pid != -1){
        logInfo->pid = getPid();
        logInfo->tid = getTid();
        logInfo->mainPid = getMainPid();
    }
}

void AppFileLog::formatLog(const LogEntry* logInfo, const char* logMsg, BufferUtils *buffer) {
    if(buffer == NULL){
        return ;
    }
    //ensure size is enought
    if(buffer->capacity() < MAX_LINE_LEN){
        buffer->setCapacity(MAX_LINE_LEN);
    }
    char *logData = buffer->data();
    if (NULL != logInfo) {
        std::string fileName = FileUtils::extractFileName(logInfo->fileName);
        char timeString [64] = {0};
        //format time
        time_t sec = logInfo->tv.tv_sec;
        tm tm = *localtime((const time_t*)&sec);
        snprintf(timeString, sizeof(timeString) -1, "%d-%02d-%02d %+.1f %02d:%02d:%02d.%.3ld", 
                1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday,
                tm.tm_gmtoff / 3600.0, tm.tm_hour, tm.tm_min, tm.tm_sec, logInfo->tv.tv_usec / 1000);
        //log format like android time->mainpid->tid->loglevel->log tag->file name->function name -> line ->msg
        snprintf(logData,buffer->capacity(),"%s %d %d %s %s: [%s %s %d] %s",timeString,logInfo->mainPid,logInfo->tid,
                levelStrings[logInfo->level], logInfo->tag == NULL?"sklog":logInfo->tag,
                fileName.c_str(), logInfo->funcName, logInfo->line,
                logMsg == NULL?"log msg is NULL":logMsg);
    }else{
        //just append log msg
        if(logMsg != NULL){
            buffer->append(logMsg,strlen(logMsg));
        }
    }
}

static const char *AssertLogMsg = "ASSERT ERROR:";
void AppFileLog::assertLog(LogEntry *logInfo,const char *condition,const char *format,...){
    if(logInfo == NULL || condition == NULL){
        return ;
    }
    initEntry(logInfo);
    char tmpBuff[4096] = {0};
    char *tmpPtr = tmpBuff;
    strcpy(tmpPtr,AssertLogMsg);
    tmpPtr += strlen(AssertLogMsg);
    strcpy(tmpPtr,condition);
    tmpPtr += strlen(condition);
    if(format != NULL){
        va_list valist;
        va_start(valist,format);
        vsnprintf(tmpPtr,sizeof(tmpBuff) -1 + tmpBuff - tmpPtr,format,valist);
        va_end(valist);
    }

    formatLog(logInfo,tmpBuff,mBuffer);
    if(mFile != NULL){
        mFile->write(*mBuffer);
    }
    if(mConsole){
        //log to terminal
        printConsole(logInfo,mBuffer->data());
    }
    assert(false);
}

void AppFileLog::printLog(LogEntry *logInfo,const char *format,...){
    if(format == NULL || logInfo == NULL){
        return ;
    }
    initEntry(logInfo);
	va_list valist;
	va_start(valist,format);
    char tmpBuff[4096] = {0};
    vsnprintf(tmpBuff,sizeof(tmpBuff) -1,format,valist);
	va_end(valist);
    formatLog(logInfo,tmpBuff,mBuffer);
    if(mFile != NULL){
        mFile->write(*mBuffer);
    }
    if(mConsole){
        //log to terminal
        printConsole(logInfo,tmpBuff);
    }
}


void AppFileLog::close(){
    AutoMutex _l(mMutex);
    if(mFile != NULL){
        delete mFile;
        mFile = NULL;
    }
}

void AppFileLog::flush(){
    if(mFile != NULL){
        mFile->flush();
    }
}
