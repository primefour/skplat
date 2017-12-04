#include"Log.h"
#include"Timers.h"
#include<unistd.h>


int main(){
    DurationTimer  timer1 ;
    timer1.start();
    sleep(1);
    timer1.stop();
    ALOGD("%lld \n",timer1.durationUsecs());
}
