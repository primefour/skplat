#include"TaskManager.h"
#include"NetworkDatabase.h"

TaskDispatch::TaskDispatch(int poolSize){
    mPoolSize = poolSize;
    mDatabase = NetworkDatabase::getInstance();
    initTasksPool();
    mHttpWorkerManager = new HttpWorkManager();
}

TaskDispatch::~TaskDispatch(){
    if(mHttpWorkerManager != NULL){
        delete mHttpWorkerManager;
        mHttpWorkerManager = NULL;
    }
}

void TaskDispatch::commitTask(sp<TaskInfo> &task){
    AutoMutex _l(mMutex);
    if(task->mPersist){//place to commit task queue first
        mDatabase->xTaskInsert(task);
    }

    if(task->mTaskType == TASK_TYPE_HTTP ||
            task->mTaskType == TASK_TYPE_HTTP_DOWNLOAD){
        mHttpWorkerManager->commitWork(task);
    }
}

void TaskDispatch::cancelTask(sp<TaskInfo> &task){
    AutoMutex _l(mMutex);
    if(task->mTaskType == TASK_TYPE_HTTP ||
            task->mTaskType == TASK_TYPE_HTTP_DOWNLOAD){
        mHttpWorkerManager->cancelWork(task);
    }

    //database
    if(task->mPersist){
        mDatabase->xTaskDelete(task->mTaskId);
    }

}

sp<TaskInfo> TaskDispatch::getTask(){
    AutoMutex _l(mMutex);
    if(mTasksPool.size() > 0){
        sp<TaskInfo> tmpTask = mTasksPool.editTop();
        mTasksPool.pop();
        return tmpTask;
    }else{
        sp<TaskInfo> tmpTask = new TaskInfo();
        return tmpTask;
    }
}

void TaskDispatch::putTask(sp<TaskInfo> &task){
    AutoMutex _l(mMutex);
    task->reset();
    if(mTasksPool.size() < mPoolSize){
        mTasksPool.push(task);
    }
}

void TaskDispatch::initTasksPool(){
    AutoMutex _l(mMutex);
    if(mTasksPool.size() == mPoolSize){
        return;
    }
    int i = 0;
    for(;i< mPoolSize ;i++){
        sp<TaskInfo> tmpTask = new TaskInfo();
        mTasksPool.push(tmpTask);
    }
}
