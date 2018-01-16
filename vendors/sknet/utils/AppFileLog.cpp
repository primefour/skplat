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
#include "Mutex.h"
#include "BufferUtils.h"


AppFileLog::AppFileLog(){
    mOpenFileTime = 0;
    mBuffer = NULL;
    mFile = NULL;
}

AppFileLog::~AppFileLog(){
    if(mFile != NULL){
        mFile->flush();
        delete mFile;
    }
}

void AppFileLog::write(const char *logMsg,int len) {
    openFile();
    if(mFile != NULL){
        mFile->write(logMsg,len);
    }
}

void AppFileLog::write(const BufferUtils &buffer) {
    openFile();
    if(mFile != NULL){
        mFile->write(buffer.data(),buffer.size());
    }
}

void AppFileLog::setDir(const char *dir){
    int ret = 0;
    if(dir == NULL){
        return;
    }
    if(!mLogDir.empty() && dir == mLogDir){
        return;
    }
    if(!FileUtils::isDir(dir)){
        ret = FileUtils::makeDir(dir);
        if(!ret){
            return;
        }
    }
    mLogDir = dir;
}

RawFile* AppFileLog::openFile() {
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
    int ret = mFile->open(O_CREAT|O_RDWR|O_APPEND);
    if(ret != OK){
        delete mFile;
        mFile = NULL;
        return NULL;
    }
    return mFile;;
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
