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

struct TaskInfo;

struct TaskObserver:public RefBase{
    virtual void onTaskDone(TaskInfo *task) = 0;
    virtual void onTaskFailed(TaskInfo *task) = 0;
    virtual void onTaskCanceled(TaskInfo *task) = 0;
    virtual void onTaskStart(TaskInfo *task) = 0;
    virtual void onTaskStates(TaskInfo *task,int progress,int arg1,int arg2)=0;
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
    TaskInfo();
    void reset(){
        //set default value
        mSendOnly = false;
        mPersist = false;
        mMethod = TASK_METHOD_HTTP_GET;
        mRetryTimes = RETRY_DEFAULT_TIMES;
        mTaskType = TASK_TYPE_HTTP;
        mConnTimeout = CONNECT_DEFAULT_TIMEOUT;//15s
        mTaskTimeout = TASK_DEFAULT_TIMEOUT;//1min
        mTaskState = TASK_STATE_IDLE;
        mTryTimes = 0;
        mStartTime = 0;
        mStartConnTime = 0;
        mRecvData = new BufferUtils();
        mSendData = new BufferUtils();
        mHttpTransfer = NULL;
        mWorkUnit = NULL;
        mListener = NULL;
        mCanceled = false;

    }

    void onStatesChange(int state,int progress,int arg1,int arg2){
        if(mTaskState == state){
            return;
        }

        if(mListener == NULL){
            return;
        }

        if(mTaskState == TASK_STATE_INIT){
            mListener->onTaskStart(this);
        }else if(mTaskState == TASK_STATE_DONE){
            mListener->onTaskDone(this);
        }else if(mTaskState == TASK_STATE_FAIL){
            mListener->onTaskFailed(this);
        }else if(mCanceled){
            mListener->onTaskCanceled(this);
        }else{
            //progress
            mListener->onTaskStates(this,progress,arg1,arg2);
        }

    }

    void cancel(){
        if(!mCanceled){
            mCanceled = true;
            if(mHttpTransfer != NULL){
                mHttpTransfer->cancel();
            }
        }
    }
};

class HttpWorkUnit:public WorkQueue::WorkUnit {
    public:
        HttpWorkUnit(sp<TaskInfo> &task):mTask(task){
        }

        HttpWorkUnit(){mTask = NULL;}

        ~HttpWorkUnit(){mTask = NULL;}

        virtual bool run(){
            return false;
        }

        virtual void cancel(){
            if(mTask != NULL){
                mTask->cancel();
            }
        }
    private:
        sp<TaskInfo> mTask;
};
#endif//
