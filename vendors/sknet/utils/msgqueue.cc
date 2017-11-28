/*******************************************************************************
 **      Filename: utils/msgqueue/msgqueue.cc
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2017-04-07 10:04:40
 ** Last Modified: 2017-04-07 10:04:40
 ******************************************************************************/
#include "thread.h"
#include "msgqueue.h"
#include <sstream>

MsgQueueWrapper::MsgQueueWrapper(std::string &name){
    need_thread = true;
    msg_queue = NULL;
    msg_thread = NULL;
    q_name = name;
}

MsgQueueWrapper::MsgQueueWrapper(bool thread,std::string &name){
    need_thread = thread;
    msg_queue = NULL;
    msg_thread = NULL;
    q_name = name;
}

void MsgQueueWrapper::msg_thread_loop(){
    if(msg_queue != NULL){
        msg_queue->enter_timer_loop();
        skdebug("msg queue exit");
    }
}

void MsgQueueWrapper::comsume_msg(){
    if(msg_queue != NULL && !need_thread){
        msg_queue->do_task();
    }else{
        skwarn("need_thread is enable ,don't call this interface to do with msg");
    }
}

MsgQueueWrapper::~MsgQueueWrapper(){
    reset_msg_queue();
}

void MsgQueueWrapper::init_msg_queue(){
    ScopedLock scopedLock(mutex);
    if(msg_queue == NULL){
        msg_queue = new MsgQueue(q_name);
        handler_seq = 1;
        int handler_id = DEFAULT_HANDLER_ID;
        std::stringstream handler_name_stream;
        handler_name_stream << q_name << handler_id;
        std::string name = handler_name_stream.str();
        DefaultHandler *handler = new DefaultHandler(handler_id,name);
        handler->SetId(DEFAULT_HANDLER_ID);
        msg_queue->add_handler(handler);
    }

    if(msg_thread == NULL && need_thread){
        msg_thread = new Thread();
        msg_thread->start(this);
    }
}

void MsgQueueWrapper::wait_done(int64_t msg_id,int handler_id){
    skdebug("msg id %" PRId64 "handler id %d ",msg_id,handler_id);
    if(msg_queue != NULL){
       msg_queue->wait_msg_done(msg_id,handler_id);
    }
    skdebug("exit msg id %" PRId64 "handler id %d ",msg_id,handler_id);
}

void MsgQueueWrapper::wait_done(int handler_id){
    skdebug("handler id %d ",handler_id);
    if(msg_queue != NULL){
        msg_queue->wait_all_msg_done(handler_id);
    }
    skdebug("exit handler id %d ",handler_id);
}


void MsgQueueWrapper::wait_done_by_seq(int64_t seq){
    skdebug("msg seq  %" PRId64 ,seq);
    if(msg_queue != NULL){
       msg_queue->wait_done_seq_done(seq);
    }
    skdebug("exit msg seq  %" PRId64 ,seq);
}

void MsgQueueWrapper::reset_msg_queue(){
    ScopedLock scopedLock(mutex);
    if(msg_queue != NULL){
        msg_queue->quit();
    }
    if(msg_queue != NULL){
        delete msg_queue;
        msg_queue = NULL;
    }
    if(msg_thread != NULL && need_thread){
        msg_thread->join();
        delete msg_thread ;
        msg_thread = NULL;
    }
}

int64_t MsgQueueWrapper::send_msg(Msg *msg){
    if(msg_queue != NULL){
        return msg_queue->send_msg(msg);
    }else{
        init_msg_queue();
        return msg_queue->send_msg(msg);
    }
}

int64_t MsgQueueWrapper::send_msg(int64_t msg_id,int handler_id){
    if(msg_queue != NULL){
        return msg_queue->send_msg(msg_id,handler_id);
    }else{
        init_msg_queue();
        return msg_queue->send_msg(msg_id,handler_id);
    }
}

int64_t MsgQueueWrapper::send_msg(int64_t msg_id,int handler_id ,int64_t after){
    if(msg_queue != NULL){
        return msg_queue->send_msg(msg_id,handler_id,after);
    }else{
        init_msg_queue();
        return msg_queue->send_msg(msg_id,handler_id,after);
    }
}

void MsgQueueWrapper::cancel_msg(int64_t msg_id,int handler_id){
    if(msg_queue != NULL){
        msg_queue->cancel_msg(msg_id,handler_id);
    }
}

void MsgQueueWrapper::cancel_msg(int handler_id){
    if(msg_queue != NULL){
        msg_queue->cancel_msg(handler_id);
    }
}

void MsgQueueWrapper::cancel_msg(Msg&msg){
    if(msg_queue != NULL){
        msg_queue->cancel_msg(msg);
    }
}

void MsgQueueWrapper::cancel_msg_seq(int64_t seq){
    if(msg_queue != NULL){
        msg_queue->cancel_msg_seq(seq);
    }
}


int MsgQueueWrapper::add_handler(Handler *handler){
    int handler_id = new_handlerid();
    handler->SetId(handler_id);
    if(msg_queue != NULL){
        msg_queue->add_handler(handler);
    }else{
        init_msg_queue();
        msg_queue->add_handler(handler);
    }
    return handler->handler_id;
}


void MsgQueueWrapper::remove_handler(int handler_id){
    if(msg_queue != NULL){
        msg_queue->remove_handler(handler_id);
    }else{
        skwarn("please initialize msg queue before do anything");
    }
}

int MsgQueueWrapper::new_handlerid(){
    if(msg_queue != NULL){
        return ((long)msg_queue&0xffff<<16)|(atomic_inc32(&handler_seq)&0xffff);
    }else{
        init_msg_queue();
        return ((long)msg_queue&0xffff<<16)|(atomic_inc32(&handler_seq)&0xffff);
    }
}

int MsgQueueWrapper::add_handler(){
    int handler_id = new_handlerid();
    std::stringstream handler_name_stream;
    handler_name_stream << q_name << handler_id;
    std::string name = handler_name_stream.str();
    DefaultHandler *handler = new DefaultHandler(handler_id,name);
    if(msg_queue != NULL){
        msg_queue->add_handler(handler);
    }else{
        init_msg_queue();
        msg_queue->add_handler(handler);
    }
    return handler->handler_id;
}

std::string default_msg_queue_name = "default_msg_queue";
MsgQueueWrapper sg_default_msg(default_msg_queue_name);

MsgQueueWrapper& getDefaultMsgQ(){
    return sg_default_msg;
}

