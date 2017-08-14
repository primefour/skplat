/*******************************************************************************
 **      Filename: log/logfilectrl.h
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2017-03-31 14:40:59
 ** Last Modified: 2017-03-31 14:40:59
 ******************************************************************************/
#ifndef __LOG_FILE_CTRL_H__
#define __LOG_FILE_CTRL_H__
#include<stdio.h>
#include<time.h>
#include "mutex.h"
#include "strutils.h"
#include "logencoder.h"

class LogFileCtrl{
    public:
        LogFileCtrl();
        ~LogFileCtrl();
        void SetLogDir(const std::string& log_dir);
        void PrintfLog(const char* _tips_format, ...); 
        void WriteLog(const char* _log);
        void CloseLogFile(); 
        void Flush();
    private:
        //don't use mutex in these function
        void Log2File(const void* _data, size_t _len);
        FILE * GetLogFileHandler();
        bool WriteFile(const void* _data, size_t _len, FILE* _file); 
        FILE *OpenLogFile(const std::string& _log_dir); 
    private:
        std::string mLogDir;
        std::string mLastLogFileName;
        std::string mLogFileName;
        Mutex mMutex;
        LogEncoder mLogEncoder;
        char * mEncodeBuff;
        FILE * mLogFileHandler;
        time_t mOpenFileTime;
        time_t mLastFileTime;
};
#endif
