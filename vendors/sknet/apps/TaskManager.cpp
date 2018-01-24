#include"TaskManager.h"
#include"NetworkDatabase.h"
#include"Timers.h"
#include"mbedtls/sha256.h"
#include"mbedtls/base64.h"

TaskDispatch::TaskDispatch(int poolSize,std::string uuid){
    mUUID = uuid;
    mPoolSize = poolSize;
    mDatabase = NetworkDatabase::getInstance();
    initTasksPool();
    mHttpWorkerManager = new HttpWorkManager();
}

TaskDispatch::~TaskDispatch(){
    ALOGD("destory");
    if(mHttpWorkerManager != NULL){
        delete mHttpWorkerManager;
        mHttpWorkerManager = NULL;
    }
    mTasksPool.clear();
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
        tmpTask->mTaskId = getIntegerId();
        mTasksPool.pop();
        return tmpTask;
    }else{
        sp<TaskInfo> tmpTask = new TaskInfo();
        tmpTask->mTaskId = getIntegerId();
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

std::string TaskDispatch::getIntegerId(){
    int64_t now =  systemTime(); //ns
    now = now/1000;//microsecond
    char buff[64] ={0};
    unsigned char id[32] ={0};
    snprintf(buff,sizeof(buff)-1,"%" PRId64"",now);
    std::string tmpId(buff,strlen(buff));
    return tmpId;
}

/*
 *#include <inttypes.h>
 *int64_t t;
 *printf("%" PRId64 "\n", t);
 *for uint64_t type:
 *#include <inttypes.h>
 *uint64_t t;
 *printf("%" PRIu64 "\n", t);
 */

std::string TaskDispatch::getStringId(){
    int64_t now =  systemTime(); //ns
    now = now/1000;//microsecond
    char buff[64] ={0};
    unsigned char id[32] ={0};
    snprintf(buff,sizeof(buff)-1,"%s%" PRId64"",mUUID.c_str(),now);
    mbedtls_sha256((unsigned char *)buff,strlen(buff),id,0);
    memset(buff,0,sizeof(buff));
    size_t len = 0;
    mbedtls_base64_encode((unsigned char *)buff,sizeof(buff),&len,(unsigned char *)id,sizeof(id));
    std::string tmpId(buff,len);
    return tmpId;
}
