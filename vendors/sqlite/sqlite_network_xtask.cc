#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<string>
#include<vector>
#include"sqlite3.h"
#include"sqlite_wrapper.h"
#include"sqlite_network_xtask.h"

/*
 * task_id: primery key,md5 value of host+port+path+timestamp
 * module_name:name of module use this function,for the callback
 * host:server host name
 * path:the server path to query 
 * data:the data will upload
 * port:server port
 * task_state:the state of task
 * percent:the process of task complete
 * send_only:only send data,no response
 * retry_times:retry times if fail
 * save_path:the save path for download file 
 * offset_start:the start point to download
 * offset_length:the length should download
 * start_time:the time when start this task
 * conn_time:the time of connect to server
 * conn_timeout:the timeout of connect
 * task_timeout:the timeout of task complete
 */
static int xtask_table_callback(void*p,sqlite3_stmt *pStmt){

    if(p == NULL){
        return -1;
    }
    std::vector<task_info> *ptr_tasks = (std::vector<task_info>*)p;
    task_info entry; 
    int i = 0;
    //task id 
    entry.task_id = std::string((const char *)sqlite3_column_text(pStmt,i++));
    //module name
    entry.module_name = std::string((const char *)sqlite3_column_text(pStmt,i++));
    //host text
    entry.host = std::string((const char *)sqlite3_column_text(pStmt,i++));
    //access path text
    entry.access_path = std::string((const char *)sqlite3_column_text(pStmt,i++));
    //where recv file to save 
    entry.save_file = std::string((const char *)sqlite3_column_text(pStmt,i++));
    //the path of file will send
    entry.send_file = std::string((const char *)sqlite3_column_text(pStmt,i++));
    //need send data
    int len = sqlite3_column_bytes(pStmt,i);
    if(len > 0){
        char *tmp_data = (char *)sqlite3_column_blob(pStmt,i++);
        entry.send_data.update(tmp_data,len);
    }else{
        i++;
    }
    //port
    entry.port = short(sqlite3_column_int(pStmt,i++));
    //send only
    entry.send_only = sqlite3_column_int(pStmt,i++) != 0;
    //retry time
    entry.retry_times= sqlite3_column_int(pStmt,i++);
    //task type
    entry.task_type= sqlite3_column_int(pStmt,i++);
    //the timeout limit of connect to server 
    entry.conn_timeout = sqlite3_column_int64(pStmt,i++);
    //the timeout limit of connect to server 
    entry.task_timeout = sqlite3_column_int64(pStmt,i++);
    //add to vector
    ptr_tasks->push_back(entry);
    return 1;
}

int xtask_insert_task(SQLite *sqlitedb, task_info *entry){
    char sql_buff[4096*2]={0};

    if(entry == NULL|| 
            entry->task_id.empty() || 
            entry->host.empty() || 
            entry->module_name.empty()){
        skerror("invalidate task,please check host module name and task id ");
        return -1;
    }
    snprintf(sql_buff,sizeof(sql_buff),"insert into xtask(task_id,module_name,host,path,save_file, \
            send_file,send_data,port,send_only, retry_times,task_type,conn_timeout,task_timeout)\
            values (\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',?,%d,%d,%d,%d,%ld,%ld);",
            entry->task_id.c_str(),
            entry->module_name.c_str(),
            entry->host.c_str(),
            !entry->access_path.empty()?entry->access_path.c_str():"NULL",
            !entry->save_file.empty()?entry->save_file.c_str():"NULL",
            !entry->send_file.empty()?entry->send_file.c_str():"NULL",
            entry->port,
            entry->send_only?1:0,
            entry->retry_times > 0 ?entry->retry_times:RETRY_DEFAULT_TIMES,
            entry->task_type,
            entry->conn_timeout > 0 ?entry->conn_timeout:CONNECT_DEFAULT_TIMEOUT, //15s
            entry->task_timeout > 0 ?entry->conn_timeout:TASK_DEFAULT_TIMEOUT//1min
            );
    sqlite3_stmt* pStmt = sqlitew_prepare_sql(sqlitedb,sql_buff);
    sqlite3_bind_blob(pStmt,1,entry->send_data.xdata,entry->send_data.xlength,SQLITE_TRANSIENT);
    int rc = sqlite3_step(pStmt);
    sqlite3_finalize(pStmt);
    if(rc != SQLITE_DONE){
        skerror("sqlite sql rc = %d, %s error msg %s",rc,sql_buff,sqlite3_errmsg(sqlitedb->db));
        return -1;
    }
    return 0;
}


int xtask_todo_count(SQLite *sqlitedb){
    char sql_buff[4096*2]={0};
    int count = 0;
    snprintf(sql_buff,sizeof(sql_buff),"select count(*) from xtask where process = 0 ;");
    sqlite3_stmt* pStmt = sqlitew_prepare_sql(sqlitedb,sql_buff);
    int rc = sqlite3_step(pStmt);
    if(SQLITE_ROW == rc ){
        count = sqlite3_column_int(pStmt,0);
        skinfo("sqlite todo task count is %d \n",count);
    }
    sqlite3_finalize(pStmt);
    return count;
}



int xtask_get_todo_tasks(SQLite * sqlitedb,std::vector<task_info> &tasks){
    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select task_id ,module_name ,"
                                    "host,port,path,send_data,task_type,send_only,"
                                    "save_file,send_file,retry_times,conn_timeout,task_timeout where process = 0 ;");
    int rc = sqlitew_exec_sql(sqlitedb,sql_buff,&tasks,xtask_table_callback);

    if(rc != SQLITE_OK){
        skerror("query fail %s ",sql_buff);
        return -1;
    }else{
        return tasks.size();
    }
}

int xtask_delete_task(SQLite *sqlitedb,std::string task_id){
    char sql_buff[4096]={0};
    if(task_id.empty()){
        return -1;
    }
    snprintf(sql_buff,sizeof(sql_buff),"delete from xtask where task_id = \'%s\';",task_id.c_str());
    char* zErr = NULL;
    int rc = sqlite3_exec(sqlitedb->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("error sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}


int xtask_update_process(SQLite * sqlitedb,std::string task_id,int process){
    char sql_buff[4096]={0};
    if(task_id.empty()){
        return -1;
    }
    snprintf(sql_buff,sizeof(sql_buff),"update xtask set process = %d where task_id = \'%s\';",process,task_id.c_str());
    char* zErr = NULL;
    int rc = sqlite3_exec(sqlitedb->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("error sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}
