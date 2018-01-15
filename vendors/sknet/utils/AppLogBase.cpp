#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "AppFileLog.h"
#include "AppLogBase.h"

static AppFileLog gLogWriter;

void skLogSetDir(const char *dir){
    gLogWriter.setDir(dir);
}

void skLogSetLevel(int level){
    gLogWriter.setLevel(level);
}

bool skLogCheckLevel(int level){
    return gLogWriter.isLog(level);
}

void skLogClose(){
    gLogWriter.close();
}

void skLogFlush(){
    gLogWriter.flush();
}

void skLogPrint(LogEntry *logInfo,const char* format, ...){
	va_list valist;
	va_start(valist,format);
    gLogWriter.printLog(logInfo,format,valist);
	va_end(valist);
}

void skLogAssert(LogEntry *logInfo,const char *condition,const char* format, ...){
	va_list valist;
	va_start(valist,format);
    gLogWriter.assertLog(logInfo,condition,format,valist);
	va_end(valist);
}

