#ifndef __TASK_MANAGER_H__
#define __TASK_MANAGER_H__
#include"Mutex.h"
#include"List.h"
#include"RefBase.h"
#include"WorkQueue.h"
#include"TaskInfo.h"
#include"NetworkDatabase.h"
#include"Vector.h"
#include"Timers.h"
#include"mbedtls/sha256.h"

//this is http work manager
class HttpWorkManager:public WorkQueue {
    public:
        HttpWorkManager(size_t maxThreads = 3):WorkQueue(maxThreads){};
        virtual ~HttpWorkManager(){
        }
        /*
         * commit a task,will create a workunit to complete this task
         */
        void commitWork(sp<TaskInfo> &task){
            task->mWorkUnit = new HttpWorkUnit(task);
            schedule(task->mWorkUnit);
        }

        /*
         * cancel a task,if found,will return OK
         * else return NAME_NOT_FOUND
         */
        bool cancelWork(sp<TaskInfo> &task){
            return cancel(task->mWorkUnit) == OK;
        }
};

/*
 * task dispatch is to operate database and dispatch
 * tasks by their type
 */
class TaskDispatch{
    public:
        /*
         * pool size default is ten
         * uuid is identify for user and help to create taskid
         */
        TaskDispatch(int poolSize = 10,std::string uuid ="testuser");

        virtual ~TaskDispatch();

        /*
         * commit tasks and will dispatch to work manager by their type
         */
        void commitTask(sp<TaskInfo> &task);

        /*
         * cancel a task
         */
        void cancelTask(sp<TaskInfo> &task);
        /*
         * get an empty task from pool
         */
        sp<TaskInfo> getTask();
        /*
         * put the useless task to pool for reusing it
         */
        void putTask(sp<TaskInfo> &task);

    private:
        /*
         * create a global unique string to identify a task
         */
        std::string getStringId();
        /*
         * create a unique string id of current user to identify a task
         */
        std::string getIntegerId();

        void initTasksPool();
        //empty taskInfo Pool
        int mPoolSize;
        //use 32 char
        std::string mUUID;
        Vector<sp<TaskInfo> > mTasksPool;
        //database manager
        sp<NetworkDatabase> mDatabase;
        HttpWorkManager *mHttpWorkerManager;
        Mutex mMutex;
};
#endif
