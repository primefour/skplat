#ifndef __HTTP_WORK_TASK_MANAGER_H__
#define __HTTP_WORK_TASK_MANAGER_H__

class HttpWorkTaskMananger:public WorkQueue {
    class HttpTask:WorkUnit {
        virtual bool run(){
            return false;
        }
        private:
            sp<TaskInfo> mTask;
    };

    public commitWork(sp<TaskInfo> &task);
    public cancelWork(sp<TaskInfo> &task);
};
#endif //__HTTP_WORK_TASK_MANAGER_H__
