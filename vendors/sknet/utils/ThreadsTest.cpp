#include <unistd.h>
#include <stdio.h>
#include "Threads.h"
#include "Log.h"

class WorkThread:public Thread {
    public:
        WorkThread(bool weakLife = false){
            ALOGD("work thread constructor");
            if(weakLife){
                extendObjectLifetime(OBJECT_LIFETIME_WEAK);
            }
        }
        WorkThread(const WorkThread & wt){
            ALOGD("work thread copy constructor");
        }
        ~WorkThread(){
            ALOGD("work thread desconstrutor");
        }
    private:
        virtual bool threadLoop() {
            ALOGD("WORK THREAD WORK THREAD ++");
            sleep(1);
            return true;
        }
};







int main(){
    //join for exit thread
    WorkThread *wt = new WorkThread();
    wt->run();
    ALOGD("wait for join");
    wt->join();
    //join and wait for thread exit
    sp<WorkThread> st = new WorkThread();
    st->run();
    ALOGD("wait for join");
    sleep(1);
    st->requestExitAndWait();
    return 0;
}
