#ifndef __APP_LOG_BASE_H__
#define __APP_LOG_BASE_H__
#include"AppFileLog.h"

struct ScopeProfile{
    ScopeProfile(int level, const char* tag,const char* fileName, const char* funcName, int line){
        if(skCheckLogLevel(level)){
            mLogInfo.level = level;
            mLogInfo.tag = tag;
            mLogInfo.fileName = fileName;
            mLogInfo.funcName = funcName;
            mLogInfo.line = line;
            gettimeofday(&mLogInfo.tv, NULL);
            mTimeValue = mLogInfo.tv;
            char logMsg[128] ={0};
            snprintf(logMsg, sizeof(logMsg) -1, "enter");
            skLogPrint(&mLogInfo,logMsg);
        }
    }
    ~ScopeProfile(){
        if(skCheckLogLevel(mLogInfo.level)){
            timeval tv;
            gettimeofday(&tv, NULL);
            mLogInfo.tv = tv;
            char logMsg[128] ={0};
            long timeSpan = (tv.tv_sec - mTimeValue.tv_sec) * 1000 + (tv.tv_usec - mTimeValue.tv_usec) / 1000;
            snprintf(strout, sizeof(strout), "exit span is :%ld ms",timeSpan);
            skLogPrint(&mLogInfo,logMsg);
        }
    }
    LogEntry mLogInfo;
	struct timeval mTimeValue;
};

void skLogPrint(LogEntry *logInfo,const char* format, ...);
void skLogAssert(LogEntry *logInfo,const char *condition,const char* format, ...);
void skLogSetDir(const char *dir);
void skLogSetLevel(int level);
bool skLogCheckLevel(int level);
void skLogClose();
void skLogFlush();

	
#endif