#ifndef __TASK_MANAGER_H__
#define __TASK_MANAGER_H__
#include"Mutex.h"
#include"List.h"
#include"TaskInfo.h"
#include"RefBase.h"

class TaskListener :RefBase{
    virtual void onTaskDone(sp<TaskInfo> &task){};
    virtual void onTaskFailed(sp<TaskInfo> &task){};
    virtual void onTaskCanceled(sp<TaskInfo> &task){};
    virtual void onTaskStart(sp<TaskInfo> &task){};
    virtual void onTaskStates(sp<TaskInfo> &task,int progress,int arg1,int arg2){};
    virtual std::string moduleName(){return "defaultTaskListener"};
};

/*
 * task manager work is to operate database and dispatch
 * tasks by their type
 */
class TaskManager:public Thread{
    public:
        virtual bool threadLoop(){
            while(1){
                mMutex.lock();
                while(mTodoTasks.empty()){
                    mCond.wait(mMutex);
                }

                while(mTodoTasks.empty()){
                    //send to type task manager
                }
            }
        }

        void commitTask(sp<TaskInfo> &task){
            AutoMutex _l(mMutex);
            if(task->mPersist){//place to commit task queue first
                mDatabase->xTaskInsert(*task)
            }
            //place to todo task queue directly
            mTodoTasks.push_back(task);
            mCond.signal();
        }

        void cancelTask(sp<TaskInfo> &task){
            //memory queue first
            AutoMutex _l(mMutex);
            typename List<sp<TaskInfo> >::iterator taskBegin = mTodoTasks.begin();
            typename List<sp<TaskInfo> >::iterator taskEnd = mTodoTasks.end();
            while(taskBegin != taskEnd){
                if((*taskBegin)->mTaskId == task->mTaskId){
                    typename List<sp<TaskInfo> >::iterator next =  taskBegin->erase(taskBegin);
                    taskBegin = next;
                    break;
                }else{
                    taskBegin ++;
                }
            }
            //database
            if(task->mPersist){
                mDatabase->xTaskDelete(task->mTaskId);
            }
        }

        void registerListener(TaskListener *callback){
            sp<TaskListener> listener = callback;
            typename List<sp<TaskListener> >::iterator callbackBegin = mCallbacks.begin();
            typename List<sp<TaskListener> >::iterator callbackEnd = mCallbacks.end();
            while(callbackBegin != callbackEnd){
                if((*callbackBegin)->moduleName() == callback->moduleName()){
                    typename List<sp<TaskListener> >::iterator next =  callbackBegin->erase(callbackBegin);
                    callbackBegin = next;
                    return;
                }else{
                    callbackBegin ++;
                }
            }
            mCallbacks.push_back(listener);
        }

        void taskObserver(sp<TaskInfo> &task,int progress,int arg1,int arg2){
            typename List<sp<TaskListener> >::iterator callbackBegin = mCallbacks.begin();
            typename List<sp<TaskListener> >::iterator callbackEnd = mCallbacks.end();
            while(callbackBegin != callbackEnd){
                if((*callbackBegin)->moduleName() == task->mModuleName){
                    break;
                }else{
                    callbackBegin ++;
                }
            }
            if(callbackBegin == callbackEnd){
                return ;
            }
            if(task->mTaskState == TASK_STATE_INIT){
                (*callbackBegin)->onTaskStart(task);
            }else if(task->mTaskState == TASK_STATE_DONE){
                (*callbackBegin)->onTaskDone(task);
            }else if(task->mTaskState == TASK_STATE_FAIL){
                (*callbackBegin)->onTaskFailed(task);
            }else{
                //progress
                (*callbackBegin)->onTaskStates(task,progress,arg1,arg2);
            }
        }

    private:
        //query from database and will be done with by work queue
        List<sp<TaskInfo> > mTodoTasks;
        //task result callbacks
        List<sp<TaskListener> > mCallbacks;
        //database manager
        sp<NetworkDatabase> mDatabase;

        Mutex mMutex;
        Conditon mCond;
};
#endif
