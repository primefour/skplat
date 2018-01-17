#ifndef __TASK_MANAGER_H__
#define __TASK_MANAGER_H__
#include"Mutex.h"
#include"List.h"
#include"TaskInfo.h"

class TaskListener{
    void onTaskDone();
    void onTaskCanceled();
    void onTaskCommited();
    void onTaskStates(Task &task,int progress,int arg1,int arg2);
    String getModuleName();
};

class TaskManager:public Thread{
    public:
        virtual bool threadLoop(){
            return true;
        }

        void commitTask();
        void cancelTask();
        void registerListener(TaskListener *callback);

    private:
        //commit to sqlite database and notify worker manager
        List<sp<TaskInfo> > mTodoTasks;

        //use workqueue
        List<sp<TaskInfo> > mDoneTasks;
        List<sp<TaskListener> > mCallbacks;
        Mutex mMutex;
        Conditon mCond;
};
#endif
