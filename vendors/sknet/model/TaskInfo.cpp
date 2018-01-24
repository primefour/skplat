#include "TaskInfo.h"
#include "BufferUtils.h"
#include "HttpTransfer.h"
#include "Log.h"

static Mutex gTaskMutex;
static Condition gTaskConditon;

static void notifyTask(){
    ALOGD("TASK notify .....");
    gTaskConditon.signal();
}

void TaskInfo::wait(){
    AutoMutex _l(gTaskMutex);
    while(!(mTaskState == TASK_STATE_DONE) && !(mTaskState == TASK_STATE_FAIL)){
        mWaiting = true;
        ALOGD("TASK WAIT.....");
        gTaskConditon.wait(gTaskMutex);
    }
    mWaiting = false;
}

TaskInfo::~TaskInfo(){
    ALOGD("task info destory");
}

TaskInfo::TaskInfo(){
    ALOGD("task info create");
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
    mListener = new TaskObserver();

    mHttpTransfer = NULL;
    mWorkUnit = NULL;
    mCanceled = false;
    mWaiting = false;
}

void TaskInfo::reset(){
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
    mListener = new TaskObserver();
    mHttpTransfer = NULL;
    mWorkUnit = NULL;
    mCanceled = false;
    mWaiting = false;
}

void TaskInfo::onStatesChange(int state,int progress,int arg1,int arg2){
    if(mTaskState == state){
        return;
    }
    mTaskState = state;
    ALOGD(">>>>>> mTaskState  = %d ",mTaskState);
    if(mListener == NULL){
        if(mWaiting && (mTaskState == TASK_STATE_FAIL ||
                    mTaskState == TASK_STATE_DONE)){
            notifyTask();
        }
        return;
    }
    sp<TaskInfo> task = this;

    if(mTaskState == TASK_STATE_INIT){
        mListener->onTaskStart(task);
    }else if(mTaskState == TASK_STATE_DONE){
        if(mWaiting){
            notifyTask();
        }else{
            mListener->onTaskDone(task);
        }
    }else if(mTaskState == TASK_STATE_FAIL){
        if(mWaiting){
            notifyTask();
        }else{
            mListener->onTaskFailed(task);
        }
    }else if(mCanceled){
        mListener->onTaskCanceled(task);
    }else{
        //progress
        mListener->onTaskStates(task,progress,arg1,arg2);
    }

}

void TaskInfo::cancel(){
    if(!mCanceled){
        mCanceled = true;
        if(mHttpTransfer != NULL){
            mHttpTransfer->cancel();
        }
    }
}

bool HttpWorkUnit::run(){
    //this is work to do
    ALOGE("Task id :%s url is:%s",mTask->mTaskId.c_str(),mTask->mUrl.c_str());
    mTask->mHttpTransfer = new HttpTransfer(mTask);
    if(mTask->mTaskType == TASK_TYPE_HTTP){
        mTask->mHttpTransfer->doGet(NULL);
    }else if(mTask->mTaskType == TASK_TYPE_HTTP_DOWNLOAD){
        mTask->mHttpTransfer->doDownload(NULL,"");
    }
}

void HttpWorkUnit::cancel(){
    if(mTask != NULL){
        mTask->cancel();
    }
}

void TaskObserver::onTaskDone(sp<TaskInfo> &task){
    ALOGD("TASK ID %s ",task->mTaskId.c_str());
}

void TaskObserver::onTaskFailed(sp<TaskInfo> &task){
    ALOGD("TASK ID %s ",task->mTaskId.c_str());
}

void TaskObserver::onTaskCanceled(sp<TaskInfo> &task){
    ALOGD("TASK ID %s ",task->mTaskId.c_str());
}

void TaskObserver::onTaskStart(sp<TaskInfo> &task){
    ALOGD("TASK ID %s ",task->mTaskId.c_str());
}

void TaskObserver::onTaskStates(sp<TaskInfo> &task,int progress,int arg1,int arg2){
    ALOGD("TASK ID %s ",task->mTaskId.c_str());
}
