#ifndef __SQLITE_NETWORK_XTASK_H__
#define __SQLITE_NETWORK_XTASK_H__
#include<stdio.h>
#include<vector>
#include<string>
#include<string.h>
#include<stdlib.h>
#include"sqlite_wrapper.h"


#define RETRY_DEFAULT_TIMES 5
#define CONNECT_DEFAULT_TIMEOUT 15000 //ms
#define TASK_DEFAULT_TIMEOUT 60000 //ms

enum TASK_TYPE{
    TASK_TYPE_HTTP,
    TASK_TYPE_HTTPS,
    TASK_TYPE_HTTP_DOWNLOAD,
    TASK_TYPE_HTTPS_DOWNLOAD,
    TASK_TYPE_TLS,
    TASK_TYPE_TCP,
    TASK_TYPE_MAX,
};

struct blob_data{
    blob_data(){
        xdata = NULL;
        xlength = 0;
    }

    blob_data(const char *data,int length){
        xdata = new char[length];
        assert(xdata != NULL);
        memcpy(xdata,data,length);
        xlength = length;
    }

    void update(const char *data,int length){
        if(xdata != NULL){
            delete[] xdata;
            xlength = length;
        }
        xdata = new char[length];
        assert(xdata != NULL);
        memcpy(xdata,data,length);
        xlength = length;

    }

    blob_data(const blob_data &d){
        xdata = new char[d.xlength];
        assert(xdata != NULL);
        memcpy(xdata,d.xdata,d.xlength);
        xlength = d.xlength;
    }

    char *xdata;
    int xlength;
    ~blob_data(){
        if(xdata != NULL){
            delete[] xdata;
        }
    }
};

struct task_info{
    task_info(const char *id,const char *module,const char *serv_host,const char *path):
        task_id(id),module_name(module),host(serv_host),access_path(path){
        port = 0; //server port
        send_only = 0; 
        retry_times = 0; //retry times
        task_type = 0;
        conn_timeout = 0; //ms
        task_timeout = 0; //ms
    }
    task_info(){
        port = 0; //server port
        send_only = 0; 
        retry_times = 0; //retry times
        task_type = 0;
        conn_timeout = 0; //ms
        task_timeout = 0; //ms
    }
    void set_send_data(const char *data,int len){
        send_data.update(data,len);
    }

    void set_send_file(char *file){
        send_file = file;
    }

    void set_save_file(char *file){
        save_file = file;
    }

    void set_send_only(int enable){
        send_only = enable;
    }

    void set_conn_timeout(long int time){
        conn_timeout = time;
    }

    void set_task_timeout(long int time){
        task_timeout = time;
    }

    void set_retry_times(int times){
        retry_times = times;
    }

    void set_port(int p){
        this->port = p;
    }

    void set_type(int type){
        task_type = type;
    }

    std::string task_id; //task name or id
    std::string module_name;
    std::string host; //server
    std::string access_path; //client visit path
    //file send or download
    std::string save_file; //the path download data where to save
    //send file 
    std::string send_file; //send a file to server
    blob_data send_data;
    short port; //server port
    bool send_only; 
    int retry_times; //retry times
    int task_type;
    //profile and limit for network 
    long int conn_timeout; //ms
    long int task_timeout; //ms
};

int xtask_insert_task(SQLite *sqlitedb, task_info *entry);
int xtask_todo_count(SQLite *sqlitedb);
int xtask_get_todo_tasks(SQLite * sqlitedb,std::vector<task_info> &tasks);
int xtask_delete_task(SQLite *sqlitedb,std::string task_id);
int xtask_update_process(SQLite * sqlitedb,std::string task_id,int process);
#endif
