/*******************************************************************************
 **      Filename: log/appenderctrl.cc
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2017-03-30 19:00:15
 ** Last Modified: 2017-03-30 19:00:15
 ******************************************************************************/
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/mount.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <zlib.h>
#include <string>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>  
#include <string.h>  
#include <sys/stat.h>  
#include <stdlib.h>  
#include "logbase.h"
#include "strutils.h"
#include "logfilectrl.h"
#include "bufferio.h"
#include "mutex.h"
#include "logencoder.h"
#include "fileutils.h"
#include "lock.h"
#include <assert.h>
#include <limits.h>
#include "platform.h"

#define FLUSH_COUNT_LIMIT 1
#define CRYPT_LOG 0
#define ENCODER_BUFFER_LEN 4*1024

LogFileCtrl::LogFileCtrl():mLogEncoder(){
    mLogFileHandler = NULL;
    mOpenFileTime = 0;
    mEncodeBuff = new(char[ENCODER_BUFFER_LEN]);
}

void LogFileCtrl::CloseLogFile() {
    if (NULL == mLogFileHandler){
        return;
    }
    fclose(mLogFileHandler);
    mLogFileHandler = NULL;
}

LogFileCtrl::~LogFileCtrl(){
    CloseLogFile();
    if(mEncodeBuff != NULL){
        delete[] mEncodeBuff; 
        mEncodeBuff = NULL;
    }
}


void LogFileCtrl::WriteLog(const char* _log) {
    if(CRYPT_LOG){
        size_t len = ENCODER_BUFFER_LEN;
        mLogEncoder.Write(_log, strnlen(_log,MAX_LINE_LEN),mEncodeBuff, len);
        Log2File(mEncodeBuff,len);
    }else{
        Log2File(_log, strnlen(_log,MAX_LINE_LEN));
    }
}


void LogFileCtrl::SetLogDir(const std::string& log_dir){
    if(!mLogDir.empty()){
        ConsolePrintf("old log dir is %s ",mLogDir.c_str());
    }
    ConsolePrintf("set log dir %s ",log_dir.c_str());
    ScopedLock lock_file(mMutex);
    if(log_dir.empty()){
        ConsolePrintf("log dir string is empty");
    }

    if(!mLogDir.empty() && log_dir != mLogDir){
        CloseLogFile();
    }

    int ret = check_dir(log_dir.c_str());
    if(ret < 0){
        ConsolePrintf("create log dir %s ",log_dir.c_str());
        ret = create_dir(log_dir.c_str());
    }

    if(ret < 0){
        ConsolePrintf("create log dir is fail ! %s dir is :%s ",strerror(errno),log_dir.c_str());
    }

    mLogDir = log_dir;
}

void LogFileCtrl::PrintfLog(const char* _tips_format, ...) {
    if (NULL == _tips_format) {
        return;
    }
    char tips_info[4096] = {0};
    va_list ap;
    va_start(ap,_tips_format);
    vsnprintf(tips_info, sizeof(tips_info), _tips_format, ap);
    va_end(ap);
    if(CRYPT_LOG){
        size_t len = ENCODER_BUFFER_LEN;
        mLogEncoder.Write(tips_info, strnlen(tips_info, sizeof(tips_info)),mEncodeBuff, len);
        Log2File(mEncodeBuff, len);
    }else{
        Log2File(tips_info, strnlen(tips_info, sizeof(tips_info)));
    }
}

void LogFileCtrl::Log2File(const void* _data, size_t _len) {
    static int wcount = 0;
    wcount ++;
    if (NULL == _data || 0 == _len){ 
        return;
    }
    ScopedLock lock_file(mMutex);
    FILE *handler = GetLogFileHandler();
    if(handler == NULL){
        ConsolePrintf("get log file handler is null");
        return;
    }

    if(!WriteFile(_data,_len,handler)){
        //only check directory,to prevent program from loop call
        if(check_dir(mLogDir.c_str()) < 0){
            ConsolePrintf("check dir fail,try to create log dir");
            //dir may remove by use
            CloseLogFile();
            //create dir again
            int ret = create_dir(mLogDir.c_str());
            if(ret < 0){
                ConsolePrintf("create log dir is fail ! %s dir is :%s ",strerror(errno),mLogDir.c_str());
                return ;
            }
        }
    }else{
        //auto flush log and write to file
        if(wcount % FLUSH_COUNT_LIMIT == 0){
            fflush(handler);
        }
    }
    return;
}

FILE * LogFileCtrl::GetLogFileHandler(){
    if(mLogDir.empty()){
        ConsolePrintf("log dir string is empty");
        return NULL;
    }

    int ret = 0;
    if(check_dir(mLogDir.c_str()) < 0){
        ConsolePrintf("check dir fail,try to create log dir");
        //dir may remove by use
        CloseLogFile();
        //create dir again
        ret = create_dir(mLogDir.c_str());
        if(ret < 0){
            ConsolePrintf("create log dir is fail ! %s dir is :%s ",strerror(errno),mLogDir.c_str());
            return NULL;
        }
    }

    return OpenLogFile(mLogDir);
}

bool LogFileCtrl::WriteFile(const void* _data, size_t _len, FILE* _file) {
    if (NULL == _file) {
        assert(false);
        ConsolePrintf("%s %d _file is NULL",__func__,__LINE__);
        return false;
    }
    long before_len = ftell(_file);
    if (before_len < 0){
        ConsolePrintf("%s %d _file before_len < 0",__func__,__LINE__);
        return false;
    }
    if (1 != fwrite(_data, _len, 1, _file)) {
        int err = ferror(_file);
        ConsolePrintf("log write file error:%d", err);
        return false;
    }
    return true;
}

FILE *LogFileCtrl::OpenLogFile(const std::string& _log_dir) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (NULL != mLogFileHandler) {
        time_t sec = tv.tv_sec;
        tm tcur = *localtime((const time_t*)&sec);
        tm filetm = *localtime(&mOpenFileTime);
        //one log file per day
        if (filetm.tm_year == tcur.tm_year && filetm.tm_mon == tcur.tm_mon && 
                filetm.tm_mday == tcur.tm_mday){
            return mLogFileHandler;
        }
    }

    mOpenFileTime = tv.tv_sec;
    char logfilepath[1024] = {0};
    make_logfilename(tv, _log_dir,LOG_PREFIX,LOG_EXT, logfilepath , 1024);

    char errbuff[1024]={0};
    if(mLogFileHandler != NULL){
        snprintf(errbuff,sizeof(errbuff),"next log file name is %s",logfilepath); 
        errbuff[sizeof(errbuff) -1] ='\0';
        fwrite(errbuff,strlen(errbuff),1,mLogFileHandler);
    }

    //close file
    CloseLogFile();
    FILE *tmp = fopen(logfilepath, "ab");
    if (NULL == tmp) {
        ConsolePrintf("open file error:%d %s, path:%s", errno, strerror(errno), logfilepath);
        if(mLogFileHandler != NULL){
            memset(errbuff,0,sizeof(errbuff));
            snprintf(errbuff,sizeof(errbuff),"open new log file fail%d error %s %s",errno, strerror(errno),logfilepath); 
            errbuff[sizeof(errbuff) -1]='\0';
            fwrite(errbuff,strlen(errbuff), 1,mLogFileHandler);
        }
    }
    mLogFileHandler = tmp;
    return mLogFileHandler ;
}

void LogFileCtrl::Flush(){
    if (NULL == mLogFileHandler){
        return;
    }
    ScopedLock lock_file(mMutex);
    fflush(mLogFileHandler);
}
