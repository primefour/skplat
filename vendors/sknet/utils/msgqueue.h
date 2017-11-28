#ifndef __MSG_QUEUE_H__
#define __MSG_QUEUE_H__

#include<list>
#include<iterator>
#include<stdio.h>
#include"sklog.h"
#include"mutex.h"
#include"condition.h"
#include"time_utils.h"
#include"lock.h"
#include"thread.h"
#include"anr.h"

typedef void (*task_fpn)() ;

/*
 * Don't use msg id -1
 */
#define INVALID_MSG_ID (-1)

/*
 *Default handler id 
 */
#define DEFAULT_HANDLER_ID 0

struct Tasklet{
    public:
        Tasklet(){
            TRes = NULL;
        }
        virtual void TStart(void *result) = 0;
        virtual ~Tasklet(){}
        void* TRes;
    private:
        Tasklet(const Tasklet &t);
        Tasklet& operator=(const Tasklet &t);
};

struct NullTasklet:public Tasklet{
    virtual void TStart(void *result){
        skerror("this is null task object \n");
    }
};

class TaskWrapper{
    public :
        TaskWrapper(){
            fpn_task = NULL;
            obj_task = NULL;
        }

        ~TaskWrapper(){
            if(obj_task != NULL){
                delete obj_task;
                obj_task = NULL;
            }
            fpn_task = NULL;
        }

        TaskWrapper(task_fpn pfn):fpn_task(pfn){
            obj_task = NULL;
        }


        TaskWrapper(Tasklet *tp){
            obj_task = tp;
            fpn_task = NULL;
        }

        void DoTask(){
            if(obj_task != NULL){
                obj_task->TStart(obj_task->TRes);
            }

            if(fpn_task != NULL){
                fpn_task();
            }
        }
    private:
        TaskWrapper(const TaskWrapper &tw);
        TaskWrapper& operator=(const TaskWrapper &tw);

        Tasklet *obj_task;
        task_fpn fpn_task;
};

enum MsgTimerType{
    kAfter,
    kImmediately,
};
    
struct MsgTimer{
    MsgTimer(int64_t _after)
    : type(kAfter)
        , after(_after)
        , period(0)
    {}

    MsgTimer()
    : type(kImmediately)
        , after(0)
        , period(0)
    {}

    MsgTimerType type;
    int64_t after;
    int64_t period;
};


struct Msg{
    Msg(Tasklet *k){
        task = new TaskWrapper(k);
        record_time = 0;
        msg_seq = -1;
        msg_id = INVALID_MSG_ID;
        handler_id = DEFAULT_HANDLER_ID;
        is_runable = true;
        broadcast = false;
        is_handling = false;
    }

    Msg(Tasklet *k,int64_t _after):timer(_after){
        task = new TaskWrapper(k);
        record_time = 0;
        msg_seq = -1;
        msg_id = INVALID_MSG_ID;
        handler_id = DEFAULT_HANDLER_ID;
        if (kImmediately != timer.type) {
            record_time = ::gettickcount();
        }
        is_runable = true;
        broadcast = false;
        is_handling = false;
    }

    Msg(task_fpn f){
        task = new TaskWrapper(f);
        record_time = 0;

        msg_seq = -1;
        msg_id = INVALID_MSG_ID;
        handler_id = DEFAULT_HANDLER_ID;
        if (kImmediately != timer.type) {
            record_time = ::gettickcount();
        }

        if(f != NULL){
            is_runable = true;
        }else{
            is_runable = false;
        }
        broadcast = false;
        is_handling = false;
    }

    Msg(task_fpn f,int64_t _after):timer(_after){
        task = new TaskWrapper(f);
        record_time = 0;
        msg_seq = -1;
        msg_id = INVALID_MSG_ID;
        handler_id = DEFAULT_HANDLER_ID;
        if (kImmediately != timer.type) {
            record_time = ::gettickcount();
        }

        if(f != NULL){
            is_runable = true;
        }else{
            is_runable = false;
        }
        broadcast = false;
        is_handling = false;
    }

    Msg(int64_t msg_id,int handler_id){
        task = NULL;
        msg_seq = -1;
        this->msg_id = msg_id;
        this->handler_id = handler_id;
        is_runable = false;
        record_time = 0;
        if (kImmediately != timer.type) {
            record_time = ::gettickcount();
        }
        broadcast = false;
        is_handling = false;
    }

    Msg(int64_t msg_id,int handler_id,int64_t after):timer(after){
        task = NULL;
        msg_seq = -1;
        this->msg_id = msg_id;
        this->handler_id = handler_id;
        record_time = 0;
        if (kImmediately != timer.type) {
            record_time = ::gettickcount();
        }
        is_runable = false;
        broadcast = false;
        is_handling = false;
    }

    virtual ~Msg(){
        if(task != NULL){
            delete task;
            task = NULL;
        }
    }


    void DoTask(){
        if(is_runable && task != NULL){
            task->DoTask();
        }
    }

    void SetId(int64_t msg_id,int handler_id){
        this->msg_id = msg_id;
        this->handler_id = handler_id;
    }

    void SetSeq(int64_t msg_seq_){
        this->msg_seq = msg_seq_;
    }

    void SetBroadcast(bool b){
        this->broadcast = b;
    }

    TaskWrapper *task;
    MsgTimer timer;
    //this is unique for msg queue 
    int64_t msg_seq;

    int64_t msg_id;
    int handler_id;
    int64_t record_time;
    bool broadcast;
    bool is_runable;
    bool is_handling;
};

struct Handler {
    Handler(std::string &name){
        h_name = name;
        handler_id = -1;
        broadcast = false;
    }

    Handler(int id,std::string &name){
        handler_id = id;
        broadcast = false;
        h_name = name;
    }

    Handler(int id,bool b,std::string &name){
        handler_id = id;
        broadcast = b;
        h_name = name;
    }

    virtual ~Handler() {
    }

    void SetId(int id){
        handler_id = id;
    }

    void SetBroadcase(bool b){
        broadcast = b ;
    }

    virtual void Handle(Msg *msg){
        msg->DoTask();
    }

    int  handler_id;
    bool broadcast;
    std::string h_name;
};


class MsgQueue{
    public:
        MsgQueue(std::string &name){
            exit = false;
            exit_done = true;
            msg_seq = 0;
            q_name = name;
        }

        ~MsgQueue(){
            exit = true;
            clear();
        }


        int64_t send_msg(Msg *msg){
            skdebug("send msg msg id %" PRId64,msg->msg_id);
            if(!has_handler(msg->handler_id)){
                skerror("there is no handler for this msg %" PRId64 " %d ",msg->msg_id,msg->handler_id);
                return -1;
            }
            //skdebug("msg mutex");
            ScopedLock scopedLock(mutex);
            msg->SetSeq(msg_seq++);
            lst_msg.push_back(msg);
            msg_cond.notifyOne();
            //skdebug("exit msg mutex");
            return msg->msg_seq;
        }

        int64_t send_msg(int64_t msg_id,int handler_id){
            Msg *msg = new Msg(msg_id,handler_id);
            return send_msg(msg);
        }

        int64_t send_msg(int64_t msg_id,int handler_id ,int64_t after){
            Msg *msg = new Msg(msg_id,handler_id,after);
            return send_msg(msg);
        }

        void cancel_msg_seq(int64_t seq){
            //skdebug("msg mutex");
            ScopedLock scopedLock(mutex);
            if(seq > msg_seq){
                return ;
            }
            std::list<Msg*>::iterator msg_begin,msg_end;
            msg_begin = lst_msg.begin();
            msg_end = lst_msg.end();
            std::list<Msg*>::iterator erase_msg;
            for(;msg_begin != msg_end;){
                erase_msg = msg_end;
                if((*msg_begin)->msg_seq == seq){
                    if(!(*msg_begin)->is_handling){
                        erase_msg = msg_begin; 
                    }else{
                        skwarn("cancel msg is running");
                        break;
                    }
                }
                msg_begin ++;
                if(erase_msg != msg_end){
                    lst_msg.erase(erase_msg);
                    delete(*erase_msg);
                }
            }
            //skdebug("exit msg mutex");
        }


        void cancel_msg(Msg&msg){
            //skdebug("msg mutex");
            ScopedLock scopedLock(mutex);
            std::list<Msg*>::iterator msg_begin,msg_end;
            msg_begin = lst_msg.begin();
            msg_end = lst_msg.end();
            std::list<Msg*>::iterator erase_msg;
            for(;msg_begin != msg_end;){
                erase_msg = msg_end;
                if(msg.msg_id == (*msg_begin)->msg_id && 
                        msg.handler_id == (*msg_begin)->handler_id){
                    if(!(*msg_begin)->is_handling){
                        erase_msg = msg_begin; 
                    }else{
                        skwarn("cancel msg is running");
                        break;
                    }
                }
                msg_begin ++;
                if(erase_msg != msg_end){
                    lst_msg.erase(erase_msg);
                    delete(*erase_msg);
                }
            }
            //skdebug("exit msg mutex");
        }

        void cancel_msg(int handler_id){
            //skdebug("msg mutex");
            ScopedLock scopedLock(mutex);
            std::list<Msg*>::iterator msg_begin,msg_end;
            msg_begin = lst_msg.begin();
            msg_end = lst_msg.end();
            std::list<Msg*>::iterator erase_msg;
            for(;msg_begin != msg_end;){
                erase_msg = msg_end;
                if(handler_id == (*msg_begin)->handler_id){
                    if(!(*msg_begin)->is_handling){
                        erase_msg = msg_begin; 
                    }else{
                        skerror("cancel a handler,but a msg is running %" PRId64 " handler id %d",(*msg_begin)->msg_id,handler_id);
                    }
                }
                msg_begin ++;
                if(erase_msg != msg_end){
                    lst_msg.erase(erase_msg);
                    delete(*erase_msg);
                }
            }
            //skdebug("exit msg mutex");
        }

        void cancel_msg(int64_t msg_id,int handler_id){
            //skdebug("msg mutex");
            ScopedLock scopedLock(mutex);
            std::list<Msg*>::iterator msg_begin,msg_end;
            msg_begin = lst_msg.begin();
            msg_end = lst_msg.end();
            std::list<Msg*>::iterator erase_msg;
            bool found = false;
            for(;msg_begin != msg_end;){
                erase_msg = msg_end;
                if(msg_id == (*msg_begin)->msg_id && 
                        handler_id == (*msg_begin)->handler_id){
                    skinfo("found cancel msg id %" PRId64,(*msg_begin)->msg_id);
                    if(!(*msg_begin)->is_handling){
                        erase_msg = msg_begin; 
                    }else{
                        skwarn("cancel msg id %" PRId64 " is running",(*msg_begin)->msg_id);
                        break;
                    }
                }
                msg_begin ++;
                if(erase_msg != msg_end){
                    found = true;
                    lst_msg.erase(erase_msg);
                    delete(*erase_msg);
                    break;
                }
            }

            if(!found){
                skerror("cancel msg id %" PRId64 " fail",msg_id);
            }
            //skdebug("exit msg mutex");
        }

        bool has_handler(int handler_id){
            bool found = false;
            std::list<Handler*>::iterator handler_begin,handler_end;
            handler_begin = lst_handler.begin();
            handler_end = lst_handler.end();
            for(;handler_begin != handler_end;handler_begin ++){
                if(handler_id == (*handler_begin)->handler_id){
                    found = true;
                }
            }
            return found;
        }

        void add_handler(Handler *handler){
            bool found = false;
            if(handler == NULL){
                return ;
            }
            //skdebug("msg mutex");
            ScopedLock scopedLock(mutex);
            std::list<Handler*>::iterator handler_begin,handler_end;
            handler_begin = lst_handler.begin();
            handler_end = lst_handler.end();
            for(;handler_begin != handler_end;handler_begin ++){
                if(handler->handler_id == (*handler_begin)->handler_id){
                    found = true;
                }
            }
            if(!found){
                lst_handler.push_back(handler);
            }
            //skdebug("msg mutex exit");
        }

        void _remove_handler(int handler_id){
            //remove handler first
            std::list<Handler*>::iterator handler_begin,handler_end;
            handler_begin = lst_handler.begin();
            handler_end = lst_handler.end();
            std::list<Handler*>::iterator erase_item;
            for(;handler_begin != handler_end;){
                erase_item = handler_end;
                if(handler_id == (*handler_begin)->handler_id){
                    erase_item = handler_begin;
                }
                handler_begin ++;
                if(erase_item != handler_end){
                    lst_handler.erase(erase_item);
                    delete (*erase_item);
                }
            }

            //cancel all msg of this handler
            std::list<Msg*>::iterator msg_begin,msg_end;
            msg_begin = lst_msg.begin();
            msg_end = lst_msg.end();
            std::list<Msg*>::iterator erase_msg;
            for(;msg_begin != msg_end;){
                erase_msg = msg_end; 
                if(handler_id == (*msg_begin)->handler_id){
                    erase_msg = msg_begin;
                }
                msg_begin ++;
                if(erase_msg != msg_end){
                    lst_msg.erase(erase_msg);
                    delete(*erase_msg);
                }
            }
        }


        void remove_handler(int handler_id){
            ScopedLock scopedLock(mutex);
            _remove_handler(handler_id);
        }


        void clear(){
            mutex.lock();
            std::list<Handler*>::iterator handler_begin,handler_end;
            handler_begin = lst_handler.begin();
            handler_end = lst_handler.end();
            std::list<Handler*>::iterator erase_item;

            erase_item = handler_end;
            for(;handler_begin != handler_end;){
                erase_item = handler_begin;
                handler_begin ++;
                if(erase_item != handler_end){
                    lst_handler.erase(erase_item);
                    delete (*erase_item);
                }
            }

            //cancel all msg of this handler
            std::list<Msg*>::iterator msg_begin,msg_end;
            msg_begin = lst_msg.begin();
            msg_end = lst_msg.end();
            std::list<Msg*>::iterator erase_msg;
            erase_msg = msg_end; 
            for(;msg_begin != msg_end;){
                erase_msg = msg_begin;
                msg_begin ++;
                if(erase_msg != msg_end){
                    lst_msg.erase(erase_msg);
                    delete(*erase_msg);
                }
            }
            mutex.unlock();
        }

        void quit(){
            exit = true;
            send_msg(INVALID_MSG_ID,DEFAULT_HANDLER_ID);
            _wait_exit();
        }

        void notify_msg_done(){
            done_cond.notifyAll();
        }


        void wait_done_seq_done(int64_t seq){
            if(seq > msg_seq){
                return ;
            }
            //skdebug("msg mutex");
            mutex.lock();
            while(1){
                std::list<Msg*>::iterator msg_begin,msg_end;
                msg_begin = lst_msg.begin();
                msg_end = lst_msg.end();
                bool found = false;
                for(;msg_begin != msg_end;msg_begin ++){
                    if(seq == (*msg_begin)->msg_seq){
                        found = true;
                    }
                }
                if(found){
                    //skdebug("msg mutex exit");
                    done_cond.wait(mutex);
                }else{
                    break;
                }
            }
            mutex.unlock();
            //skdebug("msg mutex exit");
        }

        void wait_all_msg_done(int handler){
            //skdebug("msg mutex");
            mutex.lock();
            while(1){
                std::list<Msg*>::iterator msg_begin,msg_end;
                msg_begin = lst_msg.begin();
                msg_end = lst_msg.end();
                bool found = false;
                for(;msg_begin != msg_end;msg_begin ++){
                    if(handler == (*msg_begin)->handler_id){
                        found = true;
                        skdebug("wait for msg id %" PRId64 " be done with ",(*msg_begin)->msg_id);
                    }
                }

                if(found){
                    //skdebug("msg mutex exit");
                    done_cond.wait(mutex);
                }else{
                    break;
                }
            }
            mutex.unlock();
            //skdebug("msg mutex exit");
        }

        void wait_msg_done(int64_t msg_id,int handler_id){
            //skdebug("msg mutex");
            mutex.lock();
            while(1){
                std::list<Msg*>::iterator msg_begin,msg_end;
                msg_begin = lst_msg.begin();
                msg_end = lst_msg.end();
                bool found = false;
                for(;msg_begin != msg_end;msg_begin ++){
                    if(msg_id == (*msg_begin)->msg_id &&
                            handler_id == (*msg_begin)->handler_id){
                        found = true;
                    }
                }
                if(found){
                    //skdebug("msg mutex exit");
                    done_cond.wait(mutex);
                }else{
                    break;
                }
            }
            mutex.unlock();
            //skdebug("msg mutex exit");
        }

        void do_task(){
            while(!lst_msg.empty() && !exit){
                mutex.lock();
                exit_done = false;
                Msg *msg = lst_msg.front(); 
                std::list<Handler*>::iterator handler_begin,handler_end;
                handler_begin = lst_handler.begin();
                handler_end = lst_handler.end();
                for(;handler_begin != handler_end && !exit;handler_begin ++){
                    if(msg->handler_id == (*handler_begin)->handler_id || 
                            (msg->broadcast && (*handler_begin)->broadcast)){
                        mutex.unlock();
                        (*handler_begin)->Handle(msg);
                        mutex.lock();
                    }
                }
                lst_msg.pop_front();
                delete(msg);
                msg = NULL;
                notify_msg_done();
                mutex.unlock();
            }
            exit_done = true;
            notify_msg_done();
        }

        void enter_loop(){
            while(!exit){
                mutex.lock();
                exit_done = false;
                while(lst_msg.empty()){
                    msg_cond.wait(mutex);
                }

                Msg *msg = lst_msg.front(); 
                std::list<Handler*>::iterator handler_begin,handler_end;
                handler_begin = lst_handler.begin();
                handler_end = lst_handler.end();
                for(;handler_begin != handler_end && !exit;handler_begin ++){
                    if(msg->handler_id == (*handler_begin)->handler_id || 
                            (msg->broadcast && (*handler_begin)->broadcast)){
                        mutex.unlock();
                        (*handler_begin)->Handle(msg);
                        mutex.lock();
                    }
                }
                lst_msg.pop_front();
                delete(msg);
                msg = NULL;
                notify_msg_done();
                mutex.unlock();
            }
            exit_done = true;
            notify_msg_done();
        }


        void enter_timer_loop(){
            int64_t wait_time = 0; 
            while(!exit){
                //skdebug("msg mutex");
                mutex.lock();
                //skdebug("msg mutex");

                exit_done = false;
                if(lst_msg.empty()){
                    wait_time = 10 * 60 * 1000;
                }

                while(lst_msg.empty() || wait_time > 0){
                    //skdebug("msg mutex exit");
                    msg_cond.wait(mutex,wait_time);
                    if(lst_msg.empty()){
                        wait_time = 10 * 60 * 1000;
                    }else{
                        wait_time = 0;
                    }
                }

                std::list<Msg*>::iterator msg_begin,msg_end;
                msg_begin = lst_msg.begin();
                msg_end = lst_msg.end();

                std::list<Msg*>::iterator msg = msg_end;

                int64_t wait_time_min = 0; 
                for(;msg_begin != msg_end;msg_begin ++){
                    msg = msg_begin;
                    if (kImmediately == (*msg)->timer.type) {
                        wait_time = 0;
                        break;
                    } else if (kAfter == (*msg)->timer.type) {
                        int64_t time_cost = ::gettickspan((*msg)->record_time);
                        skdebug("timer.after = %" PRId64 " time_cost = %" PRId64 ,(*msg)->timer.after,time_cost);
                        if ((*msg)->timer.after <= time_cost) {
                            wait_time = 0;
                            break;
                        } else {
                            wait_time = (*msg)->timer.after - time_cost;
                            if(wait_time  < 0){
                                wait_time = 0;
                                break;
                            }else{
                                if(wait_time_min == 0){
                                    wait_time_min = wait_time;
                                    skdebug("queue will wait time = %" PRId64,wait_time);
                                }else{
                                    wait_time = (wait_time_min < wait_time)?wait_time_min:wait_time;
                                    skdebug("queue will wait time = %" PRId64,wait_time);
                                }
                                msg = msg_end;
                            }
                        }
                    }
                }

                if(msg == msg_end){
                    //skdebug("msg mutex exit");
                    mutex.unlock();
                    continue;
                }

                std::list<Handler*>::iterator handler_begin,handler_end;
                handler_begin = lst_handler.begin();
                handler_end = lst_handler.end();
                for(;handler_begin != handler_end && !exit;handler_begin ++){
                    if((*msg)->handler_id == (*handler_begin)->handler_id || 
                            ((*msg)->broadcast && (*handler_begin)->broadcast)){
                        (*msg)->is_handling = true;
                        //skdebug("msg mutex exit");
                        mutex.unlock();
                        //anr for ten minutes
                        SCOPE_ANR_AUTO(10*60*1000);
                        skdebug("msg handler is doing");
                        (*handler_begin)->Handle(*msg);
                        skdebug("msg handler is done");
                        //skdebug("msg mutex");
                        mutex.lock();
                        (*msg)->is_handling = false;
                    }
                }
                lst_msg.erase(msg);
                delete(*msg);
                notify_msg_done();
                //skdebug("msg mutex exit");
                mutex.unlock();
            }
            exit_done = true;
            notify_msg_done();
        }


        void _wait_exit(){
            mutex.lock();
            while(!exit_done){
                done_cond.wait(mutex);
            }
            mutex.unlock();
        }

    private:
        std::list<Handler*> lst_handler;
        std::list<Msg*> lst_msg;
        Mutex mutex;
        Condition done_cond;
        Condition msg_cond;
        bool exit;
        bool exit_done;
        std::string q_name;
        volatile int64_t  msg_seq;
};

class DefaultHandler:public Handler {
    public:
        DefaultHandler(int id,std::string &name):Handler(id,name){
        }
        virtual ~DefaultHandler(){ }
        virtual void Handle(Msg *msg){
            skdebug("msg.is_runable is %d handler is %s",msg->is_runable,h_name.c_str());
            if(msg->is_runable){
                msg->DoTask();
                return ;
            }
            switch(msg->msg_id){
                case INVALID_MSG_ID:
                    skerror("get a invalidate msg id-1 ");
                    break;
                default :
                    skwarn("get a unknown msg id %" PRId64, msg->msg_id);
                    break;
            }
        }
};

class MsgQueueWrapper {
    private:
        Mutex mutex;
        MsgQueue *msg_queue;
        Thread *msg_thread;
        std::string q_name;
        bool need_thread;
        volatile uint32_t handler_seq;
    private:
        void msg_thread_loop();
        int new_handlerid();
        MsgQueueWrapper(const MsgQueueWrapper& a){
            skerror("Don't assign msg queue");
        }
        MsgQueueWrapper& operator=(const MsgQueueWrapper& a){
            skerror("Don't assign msg queue");
            return *this;
        }
    public:
        void operator()(){
            msg_thread_loop();
        }
        MsgQueueWrapper(std::string &name);
        MsgQueueWrapper(bool thread,std::string &name);
        ~MsgQueueWrapper();
        void comsume_msg();
        void init_msg_queue();
        void reset_msg_queue();

        int64_t send_msg(Msg *msg);
        int64_t send_msg(int64_t msg_id,int hander_id);
        int64_t send_msg(int64_t msg_id,int handler_id ,int64_t after);

        void cancel_msg_seq(int64_t seq);
        void cancel_msg(int handler_id);
        void cancel_msg(int64_t msg_id,int handler_id);
        void cancel_msg(Msg&msg);
        int add_handler(Handler *handler);
        int add_handler();

        void wait_done_by_seq(int64_t seq);
        void wait_done(int64_t msg_id,int handler_id);
        void wait_done(int handler_id);
        void remove_handler(int handler_id);
};

MsgQueueWrapper& getDefaultMsgQ();
#endif
