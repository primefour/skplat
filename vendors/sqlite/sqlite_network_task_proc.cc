#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<string>
#include<vector>
#include"sqlite3.h"
#include"sqlite_wrapper.h"
#include"sqlite_network_xtask.h"
#include"sqlite_network_task_proc.h"


static int xprocess_table_callback(void*p,sqlite3_stmt *pStmt){
    std::vector<task_process> *ptr_process = (std::vector<task_process>*)p;
    task_process entry; 
    int i = 0;
    //task id 
    entry.task_id = std::string((const char *)sqlite3_column_text(pStmt,i++));
    //task state
    entry.task_state = sqlite3_column_int(pStmt,i++);
    //percent 
    entry.complete = sqlite3_column_int(pStmt,i++);
    //has retred times 
    entry.try_times = sqlite3_column_int(pStmt,i++);
    //the time of start task
    entry.start_time = sqlite3_column_int64(pStmt,i++);
    //the time of connect to server 
    entry.conn_time = sqlite3_column_int64(pStmt,i++);
    ptr_process->push_back(entry);
}

int xprocess_insert_task(SQLite *sqlitedb,std::string task_id){
    char sql_buff[1024]={0};
    if(task_id.empty()){
        skerror("invalidate task,please check task id ");
        return -1;
    }
    snprintf(sql_buff,sizeof(sql_buff),"insert into task_proc(task_id) values (\'%s\');",task_id.c_str());
    sqlite3_stmt* pStmt = sqlitew_prepare_sql(sqlitedb,sql_buff);
    int rc = sqlite3_step(pStmt);
    sqlite3_finalize(pStmt);
    char *zErr = NULL;
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}

void xprocess_update_compete(SQLite *sqlitedb,std::string &task_id,int complete){
    char sql_buff[4096]={0};
    if(task_id.empty()){
        skerror("invalidate task,please check task id ");
        return;
    }
    snprintf(sql_buff,sizeof(sql_buff),"update task_proc set complete = %d where task_id = \'%s\';",complete,task_id.c_str());
    sqlite3_stmt* pStmt = sqlitew_prepare_sql(sqlitedb,sql_buff);
    int rc = sqlite3_step(pStmt);
    sqlite3_finalize(pStmt);

    char *zErr = NULL;
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
    }
    return;
}

void xprocess_update_status(SQLite *sqlitedb,std::string &task_id,int status){
    char sql_buff[4096]={0};
    if(task_id.empty()){
        skerror("invalidate task,please check task id ");
        return;
    }
    snprintf(sql_buff,sizeof(sql_buff),"update task_proc set task_state = %d where task_id = \'%s\';",status,task_id.c_str());
    sqlite3_stmt* pStmt = sqlitew_prepare_sql(sqlitedb,sql_buff);
    int rc = sqlite3_step(pStmt);
    sqlite3_finalize(pStmt);

    char *zErr = NULL;
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
    }
}

void xprocess_update_trytimes(SQLite *sqlitedb,std::string &task_id,int times){
    char sql_buff[4096]={0};
    if(task_id.empty()){
        skerror("invalidate task,please check task id ");
        return;
    }
    snprintf(sql_buff,sizeof(sql_buff),"update task_proc set try_times = %d where task_id = \'%s\';",times,task_id.c_str());
    sqlite3_stmt* pStmt = sqlitew_prepare_sql(sqlitedb,sql_buff);
    int rc = sqlite3_step(pStmt);
    sqlite3_finalize(pStmt);

    char *zErr = NULL;
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
    }

}

void xprocess_update_starttime(SQLite *sqlitedb,std::string &task_id,long int start_time){
    char sql_buff[4096]={0};
    if(task_id.empty()){
        skerror("invalidate task,please check task id ");
        return;
    }
    snprintf(sql_buff,sizeof(sql_buff),"update task_proc set start_time= %ld where task_id = \'%s\';",start_time,task_id.c_str());
    sqlite3_stmt* pStmt = sqlitew_prepare_sql(sqlitedb,sql_buff);
    int rc = sqlite3_step(pStmt);
    sqlite3_finalize(pStmt);

    char *zErr = NULL;
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
    }

}

void xprocess_update_conntime(SQLite *sqlitedb,std::string &task_id,long int conn_time){
    char sql_buff[4096]={0};
    if(task_id.empty()){
        skerror("invalidate task,please check task id ");
        return;
    }
    snprintf(sql_buff,sizeof(sql_buff),"update task_proc set conn_time = %ld where task_id = \'%s\';",conn_time,task_id.c_str());
    sqlite3_stmt* pStmt = sqlitew_prepare_sql(sqlitedb,sql_buff);
    int rc = sqlite3_step(pStmt);
    sqlite3_finalize(pStmt);

    char *zErr = NULL;
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
    }

}

int xprocess_delete_task(SQLite *sqlitedb,std::string &task_id){
    char sql_buff[4096]={0};
    if(task_id.empty()){
        skerror("invalidate task,please check task id ");
        return -1;
    }
    snprintf(sql_buff,sizeof(sql_buff),"delete from task_proc where task_id = \'%s\';",task_id.c_str());
    sqlite3_stmt* pStmt = sqlitew_prepare_sql(sqlitedb,sql_buff);
    int rc = sqlite3_step(pStmt);
    sqlite3_finalize(pStmt);
    char *zErr = NULL;
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}

