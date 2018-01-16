

1.user interface of module

registerTaskListener

TaskListener{
    void onTaskDone(Task &task);
    void onTaskFailed(Task &task);
    void onTaskStates(Task &task,int progress,int arg1,int arg2);
    void onCancelDone()
    void onCancelFailed();
};


commitTask(Task &task);
commit a task to skplat and wait for being done
insert task to database first and trigger database listener to 
find the changes of database.
database listener will change the status of task when deal it with

cancelTask(Task &task);
cancel a task which commit before
triger cancel event first and then delete the record in the database
callback task listener 


2.client database listener

registerDBListener

DataBaseListner{
    void onTaskCommit()
    void onTaskCancel()
}


