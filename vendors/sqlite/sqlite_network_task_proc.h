#ifndef __SQLITE_TASK_PROCESS_H__
#define __SQLITE_TASK_PROCESS_H__
#include<string>
#include<stdio.h>
#include"sqlite_network_xtask.h"

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

struct task_process{
    task_info * task;
    std::string task_id;
    int task_state;
    int complete;
    int try_times;
    long int start_time;
    long int conn_time;
    long int end_time;
};

int xprocess_get_task_profile(SQLite * sqlitedb,task_process &task_profile,std::string &task_id);
int xprocess_insert_task(SQLite *sqlitedb,std::string task_id);
int xprocess_delete_task(SQLite *sqlitedb,std::string &task_id);
void xprocess_update_compete(SQLite *sqlitedb,std::string &task_id,int complete);
void xprocess_update_status(SQLite *sqlitedb,std::string &task_id,int status);
void xprocess_update_trytimes(SQLite *sqlitedb,std::string &task_id,int times);
void xprocess_update_starttime(SQLite *sqlitedb,std::string &task_id,long int start_time);
void xprocess_update_conntime(SQLite *sqlitedb,std::string &task_id,long int conn_time);

#endif //__SQLITE_TASK_PROCESS_H__

