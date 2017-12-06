#include"Looper.h"
#include"Log.h"
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<string.h>
#include<errno.h>
#include"Mutex.h"
#include"Condition.h"

int eventCallback(int fd, int events, void* data){
    ALOGD("event callback receive fd = %d events = %d data = %p ",fd,events,data);
    return 0;
}

class MyMessageHandler:public MessageHandler {
    public:
        enum {
            FIRST_MSG_ID = 1,
            SECOND_MSG_ID,
            THIRD_MSG_ID,
            FORTH_MSG_ID,
        };
        MyMessageHandler(Looper * looper):mLooper(looper){
            ALOGD("MyMessageHandler construtor");
        }

        ~MyMessageHandler(){
            ALOGD("MyMessageHandler deconstrutor");
        }
        virtual void handleMessage(const Message& message){
            int what = message.what;
            switch(what){
                case FIRST_MSG_ID:
                    ALOGD("msg FIRST_MSG_ID");
                    break;
                case SECOND_MSG_ID:
                    ALOGD("msg SECOND_MSG_ID");
                    break;
                case THIRD_MSG_ID:
                    ALOGD("msg THIRD_MSG_ID");
                    break;
                default:
                    ALOGW("unknown msg id %d ",what);
                    break;
            }
        }
        void sendMessage(const Message& msg) {
            mLooper->sendMessage(this,msg);
        }

    private:
        sp<Looper> mLooper;
};


class MyThread:public Thread {
    public:
        virtual bool threadLoop(){
            {
                AutoMutex _l(mMutex);
                ALOGD("enter thread looper");
                mLooper = Looper::prepare();
                mCondition.broadcast();
            }
            ALOGD("enter poll once");
            mLooper->pollAll(-1,NULL,NULL,NULL);
            ALOGD("exit poll once");
        }
        sp<Looper>& getLooper(){
            while(mLooper == NULL){
                mCondition.wait(mMutex);
            }
            return mLooper;
        }
    private:
        sp<Looper> mLooper;
        Mutex      mMutex;
        Condition  mCondition;
};


struct MyCmd{
    int32_t cmd;
    int32_t arg1;
    int32_t arg2;
};


static void writeEvent(int fd,int cmd,int32_t arg1 = 0,int32_t arg2 = 0){
    MyCmd tmpCmd;
    tmpCmd.cmd = cmd;
    tmpCmd.arg1 = arg1;
    tmpCmd.arg2 = arg2;
    int ret = write(fd,(void *)&tmpCmd,sizeof(MyCmd));
    if(ret < 0){
        ALOGD("fail to send a event");
    }else{
        if(ret == sizeof(MyCmd)){
            return;
        }else{
            ALOGW("there is something error for pipe");
        }
    }
}


static void readEvent(int fd,MyCmd &out){
    int ret = read(fd,(void*)&out,sizeof(MyCmd));
    if(ret < 0){
        ALOGW("fail to read a event");
    }else{
        if(ret == sizeof (MyCmd)){
            return;
        }else{
            ALOGW("there is something wrong with pipe");
        }
    }
}

static void sendCmd(int fd,int cmd){
    writeEvent(fd,cmd);
}

static int myEventCallback(int fd, int events, void* data){
    if(events == ALOOPER_EVENT_INPUT){
        ALOGD("get an input event");
        MyCmd ev;
        readEvent(fd,ev);
        ALOGD("GET EVENT %d %d %d ",ev.cmd,ev.arg1,ev.arg2);
    }
}


int main(){
    MyThread *mt = new MyThread();
    MyMessageHandler *mh = NULL;
    mt->run();
    ALOGD("thread start to run ");
    sp<Looper> &lp = mt->getLooper();
    if(lp.get() == NULL){
        ALOGE("looper get is NULL");
    }else{
        mh = new MyMessageHandler(lp.get());
        ALOGD("thread start to run ");
        Message msg(MyMessageHandler::FIRST_MSG_ID);
        mh->sendMessage(msg);
    }

    int pipefd[2]={0};

    int ret = pipe(pipefd);
    int flags = fcntl(pipefd[0],F_GETFD);
    flags |= O_NONBLOCK;
    ret += fcntl(pipefd[0],F_SETFD,flags);

    flags = fcntl(pipefd[1],F_GETFD);
    flags |= O_NONBLOCK;
    ret += fcntl(pipefd[1],F_SETFD,flags);

    if(ret < 0){
        ALOGW("fail to create pipe %s ",strerror(errno));
        return 0;
    }
    lp->addFd(pipefd[0],0,ALOOPER_EVENT_INPUT,myEventCallback,NULL);
    sendCmd(pipefd[1],3);
    mt->join();
    return 0;
}
