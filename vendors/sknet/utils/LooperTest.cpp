#include"Looper.h"
#include"Log.h"
#include<unistd.h>

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
            ALOGD("enter thread looper");
            mLooper = Looper::prepare();
            ALOGD("enter poll once");
            mLooper->pollOnce(-1,NULL,NULL,NULL);
            ALOGD("exit poll once");
        }
        sp<Looper>& getLooper(){
            return mLooper;
        }
    private:
        sp<Looper> mLooper;
};

int main(){
    MyThread *mt = new MyThread();
    MyMessageHandler *mh = NULL;
    mt->run();
    ALOGD("thread start to run ");
    sleep(1);
    sp<Looper> &lp = mt->getLooper();
    if(lp.get() == NULL){
        ALOGE("looper get is NULL");
    }else{
        mh = new MyMessageHandler(lp.get());
        ALOGD("thread start to run ");
        Message msg(MyMessageHandler::FIRST_MSG_ID);
        mh->sendMessage(msg);
    }

    mt->join();
    return 0;
}
