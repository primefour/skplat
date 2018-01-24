#ifndef __TASK_INFO_H__
#define __TASK_INFO_H__
#include<stdio.h>
#include<string>
#include<string.h>
#include<stdlib.h>
#include"BufferUtils.h"
#include"HttpTransfer.h"
#include"RefBase.h"
#include"WorkQueue.h"

#define RETRY_DEFAULT_TIMES 5
#define CONNECT_DEFAULT_TIMEOUT 15000 //ms
#define TASK_DEFAULT_TIMEOUT 60000 //ms

enum TASK_INFO_TYPE{
    TASK_TYPE_HTTP,
    TASK_TYPE_HTTP_DOWNLOAD,
    TASK_TYPE_TLS,
    TASK_TYPE_TCP,
    TASK_TYPE_MAX,
};

enum TASK_INFO_STATE {
    TASK_STATE_IDLE,
    TASK_STATE_INIT,
    TASK_STATE_CONN,
    TASK_STATE_CONNED,
    TASK_STATE_SEND,
    TASK_STATE_SENDED,
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

struct TaskInfo;

struct TaskObserver:public RefBase{
    virtual void onTaskDone(sp<TaskInfo> &task);
    virtual void onTaskFailed(sp<TaskInfo> &task);
    virtual void onTaskCanceled(sp<TaskInfo> &task);
    virtual void onTaskStart(sp<TaskInfo> &task);
    virtual void onTaskStates(sp<TaskInfo> &task,int progress,int arg1,int arg2);
};

struct TaskInfo :public RefBase{
    std::string mTaskId; //task name or id
    //request url
    std::string mUrl;
    //only for download task
    std::string mRecvFile; //the path download data where to save
    //send file
    std::string mSendFile; //send a file to server
    //send data
    sp<BufferUtils> mSendData; //data will send to server
    //write buffer
    sp<BufferUtils> mRecvData;
    //transfer
    sp<HttpTransfer> mHttpTransfer;
    //an observer
    sp<TaskObserver> mListener;
    //work unit for workqueue
    sp<WorkQueue::WorkUnit> mWorkUnit;

    bool mPersist;
    bool mSendOnly;
    int  mMethod;
    int  mRetryTimes; //retry times
    int  mTaskType;
    long mConnTimeout; //ms
    long mTaskTimeout; //ms
    //do with info
    int mTaskState;
    int mTryTimes;
    int mCanceled;
    long mStartTime;
    long mStartConnTime;
    bool mWaiting;
    TaskInfo();
    ~TaskInfo();
    void reset();
    void onStatesChange(int state,int progress,int arg1,int arg2);
    void cancel();
    void wait();
};

class HttpWorkUnit:public WorkQueue::WorkUnit {
    public:
        HttpWorkUnit(sp<TaskInfo> &task):mTask(task){
            ALOGE("Task id :%s url is:%s create",mTask->mTaskId.c_str(),mTask->mUrl.c_str());
        }
        HttpWorkUnit(){mTask = NULL;}
        ~HttpWorkUnit(){
            ALOGE("Task id :%s url is:%s destroy",mTask->mTaskId.c_str(),mTask->mUrl.c_str());
            mTask = NULL;
        }
        virtual bool run();
        virtual void cancel();
    private:
        sp<TaskInfo> mTask;
};
#endif//
