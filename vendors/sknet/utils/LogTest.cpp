#include<stdio.h>
#include"Log.h"


void TestLog(){
    const char *strTest = "Test String ";
    ALOGD("hello world log");
    ALOGD("print string %s ",strTest);
    bool testCond = false;
    ALOG_ASSERT(true,"this is false test %s ",strTest);
    LOG_ALWAYS_FATAL_IF(testCond,"fail fatal test %s ",strTest);
    fflush(stdout);
    ALOG_ASSERT(testCond,"this is false test %s ",strTest);
}


int main(){
    TestLog();
}
