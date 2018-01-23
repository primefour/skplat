#ifndef __TASK_MANAGER_H__
#define __TASK_MANAGER_H__
#include"Mutex.h"
#include"List.h"
#include"RefBase.h"
#include"WorkQueue.h"
#include"TaskInfo.h"
#include"NetworkDatabase.h"
#include"Vector.h"

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
            WorkQueue::WorkUnit *work = new HttpWorkUnit(task);
            task->mWorkUnit = work;
            schedule(work);
        }

        /*
         * cancel a task,if found,will return OK
         * else return NAME_NOT_FOUND
         */
        bool cancelWork(sp<TaskInfo> &task){
            return cancel(&(task->mWorkUnit)) == OK;
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
         */
        TaskDispatch(int poolSize = 10);

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
        void initTasksPool();
        //empty taskInfo Pool
        int mPoolSize;
        Vector<sp<TaskInfo> > mTasksPool;
        //database manager
        sp<NetworkDatabase> mDatabase;
        HttpWorkManager *mHttpWorkerManager;
        Mutex mMutex;
};
#endif
