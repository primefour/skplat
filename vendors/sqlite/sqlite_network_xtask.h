#ifndef __SQLITE_NETWORK_XTASK_H__
#define __SQLITE_NETWORK_XTASK_H__
#include<stdio.h>
#include<vector>
#include<string>
#include"sqlite_wrapper.h"



enum TASK_TYPE{
    TASK_TYPE_HTTP,
    TASK_TYPE_HTTPS,
    TASK_TYPE_HTTP_DOWNLOAD,
    TASK_TYPE_HTTPS_DOWNLOAD,
    TASK_TYPE_TLS,
    TASK_TYPE_TCP,
    TASK_TYPE_MAX,
};


enum TASK_STATE {
    TASK_STATE_IDLE,
    TASK_STATE_INIT,
    TASK_STATE_CONN,
    TASK_STATE_SEND,
    TASK_STATE_RECV,
    TASK_STATE_DONE,
    TASK_STATE_FAIL,
    TASK_STATE_MAX,
};

struct blob_data{
    blob_data(){
        xdata = NULL;
        xlength = 0;
    }

    blob_data(char *data,int length){
        xdata = new char[lehgth];
        assert(xdata != NULL);
        memncpy(xdata,data,length);
        xlength = length;
    }
    void update(char *data,int lenght){
        if(xdata != NULL){
            delete[] xdata;
            xlength = length;
        }
        xdata = new char[lehgth];
        assert(xdata != NULL);
        memncpy(xdata,data,length);
        xlength = length;
    }

    char *xdata;
    int xlength;
    ~blob_data(){
        if(xdata != NULL){
            delete[] xdata;
        }
    }
    private:
        blob_data(const blob_data &d);
};

struct task_info{
    std::string task_id; //task name or id
    std::string module_name;
    std::string host; //server
    std::string access_path; //client visit path
    blob_data send_data;
    blob_data recv_data;

    //file send or download
    std::string save_path; //the path download data where to save
    //send file 
    std::string src_path; //send a file to server
    short port; //server port
    bool send_only; 
    int task_state;
    int retry_time; //retry times
    int task_type;
    int percent;

    //send data 
    int64_t sends; //the data size of has send
    //recv data
    int64_t recvs; //data size of has recv

    int64_t conn_timeout; //ms
    int64_t task_timeout; //ms
    int64_t start_time;
    int64_t end_time;
    int64_t offset_start;
    int64_t offset_length;
    int64_t conn_time;
};
#endif
