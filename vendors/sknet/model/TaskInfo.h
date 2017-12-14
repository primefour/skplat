#ifndef __TASK_INFO_H__
#define __TASK_INFO_H__
#include<stdio.h>
#include<string>
#include<string.h>
#include<stdlib.h>
#include"BufferUtils.h"

#define RETRY_DEFAULT_TIMES 5
#define CONNECT_DEFAULT_TIMEOUT 15000 //ms
#define TASK_DEFAULT_TIMEOUT 60000 //ms

enum TASK_INFO_TYPE{
    TASK_TYPE_HTTP,
    TASK_TYPE_HTTPS,
    TASK_TYPE_HTTP_DOWNLOAD,
    TASK_TYPE_HTTPS_DOWNLOAD,
    TASK_TYPE_TLS,
    TASK_TYPE_TCP,
    TASK_TYPE_MAX,
};

enum TASK_INFO_STATE {
    TASK_STATE_IDLE,
    TASK_STATE_INIT,
    TASK_STATE_CONN,
    TASK_STATE_SEND,
    TASK_STATE_RECV,
    TASK_STATE_DONE,
    TASK_STATE_FAIL,
    TASK_STATE_MAX,
};

enum TASK_METHOD_STATE {
    TASK_METHOD_HTTP_GET,
    TASK_METHOD_HTTP_POST,
    TASK_METHOD_HTTP_PUT,
    TASK_METHOD_HTTP_DELETE,
    TASK_METHOD_MAX,
};

struct TaskInfo{
    std::string mTaskId; //task name or id
    std::string mModuleName; //for notify callback
    std::string mUrl;
    //only for download task 
    std::string mRecvFile; //the path download data where to save
    //send file 
    std::string mSendFile; //send a file to server
    //send data
    BufferUtils mSendData; //data will send to server
    //write buffer
    BufferUtils mRecvData;
    bool mSendOnly; 
    int  mMethod;
    int  mRetryTimes; //retry times
    int  mTaskType;
    long mConnTimeout; //ms
    long mTaskTimeout; //ms
    //do with info
    int mTaskState;
    int mTryTimes;
    long mStartTime;
    long mStartConnTime;
    TaskInfo(){
        //set default value
        mSendOnly = false;
        mMethod = TASK_METHOD_HTTP_GET;
        mRetryTimes = RETRY_DEFAULT_TIMES;
        mTaskType = TASK_TYPE_HTTP;
        mConnTimeout = CONNECT_DEFAULT_TIMEOUT;//15s
        mTaskTimeout = TASK_DEFAULT_TIMEOUT;//1min
        mTaskState = TASK_STATE_IDLE;
        mDone = 0;
        mTryTimes = 0;
        mStartTime = 0;
        mStartConnTime = 0;
    }
};
#endif//
