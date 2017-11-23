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
    std::vector<task_info> *ptr_dns = (std::vector<task_info>*)p;
    task_info entry; 
    int i = 0;

    //task id 
    entry->task_id = std::string(sqlite3_column_text(pStmt,i++));
    //module name
    entry->module_name = std::string(sqlite3_column_text(pStmt,i++));
    //host text
    entry->host = std::string(sqlite3_column_text(pStmt,i++));
    //access path text
    entry->access_path = std::string(sqlite3_column_text(pStmt,i++));

    //need send data
    int len = sqlite3_column_bytes(pStmt,i);
    if(len > 0){
        char *tmp_data = sqlite3_column_blob(pStmt,i++);
        entry->send_data.update(tmp_data,len);
    }else{
        i++;
    }

    //recv data
    int len = sqlite3_column_bytes(pStmt,i);
    if(len > 0){
        char *tmp_data = sqlite3_column_blob(pStmt,i++);
        entry->recv_data.update(tmp_data,len);
    }else{
        i++;
    }

    //port
    entry->port = short(sqlite3_column_int(pStmt,i++));
    //task state
    entry->task_state = sqlite3_column_int(pStmt,i++);
    //task type
    entry->task_type= sqlite3_column_int(pStmt,i++);
    //percent 
    entry->percent = sqlite3_column_int(pStmt,i++);
    //send only
    entry->send_only = sqlite3_column_int(pStmt,i++);
    //retry time
    entry->retry_time= sqlite3_column_int(pStmt,i++);
    //where recv file to save 
    entry->save_path = std::string(sqlite3_column_text(pStmt,i++));
    //the path of file will send
    entry->src_path = std::string(sqlite3_column_text(pStmt,i++));

    //file offset start point
    entry->offset_start= sqlite3_column_int64(pStmt,i++);

    //file length from offset start point
    entry->offset_length= sqlite3_column_int64(pStmt,i++);
    //the time of start task
    entry->start_time = sqlite3_column_int64(pStmt,i++);
    //the time of connect to server 
    entry->conn_time = sqlite3_column_int64(pStmt,i++);
    //the timeout limit of connect to server 
    entry->conn_timeout = sqlite3_column_int64(pStmt,i++);
    //the timeout limit of connect to server 
    entry->task_timeout = sqlite3_column_int64(pStmt,i++);
    //add to vector
    ptr_dns->push_back(entry);
}


int insert_task(SQLite sqlite, task_info * entry){
    char sql_buff[4096*2]={0};

    if(entry == NULL|| 
            entry->task_id.empty() || 
            entry->host.empty() || 
            entry->module_name.empty()){
        return -1;
    }
    snprintf(sql_buff,sizeof(sql_buff),"insert into task(task_id,module_name,host,access_path,save_path, \
        src_path,port,send_only,task_state, \
            retry_time,task_type,conn_timeout,task_timeout,start_time,end_time,\
            send_data,recv_data) values ('%s','%s','%s','%s', \
                '%s',%lld,%lld,%lld,%lld,%d,%d,%d, \
                %d,%d,%lld,%lld,%lld,%lld, \
                ?,?);",
            entry->task_id.c_str(),
            entry->module_name.c_str(),
            entry->host.c_str(),
            !entry->access_path.empty()?entry->access_path.c_str():"NULL",
            !entry->save_path.empty()?entry->save_path.c_str():"NULL",
            !entry->src_path.empty()?entry->src_path.c_str():"NULL",
            entry->port,entry->send_only,TASK_STATE_IDLE,
            entry->retry_time > 0 ?entry->retry_time:5,
            entry->task_type,
            entry->conn_timeout > 0 ?entry->conn_timeout:15000, //15s
            entry->task_timeout > 0 ?entry->conn_timeout:60000, //1min
            0,0);

    sqlite3_stmt* pStmt = sqlitew_prepare_sql(sqlitedb,sql_buff);
    sqlite3_bind_blob(pStmt, 1,entry->send_data.Len(),entry->send_data.Ptr(), NULL );
    sqlite3_bind_blob(pStmt, 2,entry->recv_data.Len(),entry->recv_data.Ptr(), NULL );
    int rc = sqlite3_step(pStmt);
    sqlite3_finalize(pStmt);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}

int check_task(SQLite sqlite,const char *task_id){
    char sql_buff[4096] ={0};
    if(task_id == NULL){
        return 0;
    }

    snprintf(sql_buff,sizeof(sql_buff),
                "select count(*) from task where task_id = '%s';",task_id);

    sqlite3_stmt *pStmt = NULL,
    int rc = sqlite3_prepare_v2(sqlite->db,sql.c_str(), -1, &pStmt,NULL);
    if(rc!=SQLITE_OK || pStmt == NULL){
        skerror("prepare sql %s  fail ",sql.c_str());
        return -1;
    }
    rc = sqlite3_step(pStmt);
    int count = 0;
    if(rc != SQLITE_ROW){
        skerror("execute sql  %s fail ",sql.c_str());
    }else{
        count = sqlite3_column_int(pStmt,0);
    }
    sqlite3_finalize(pStmt);
    return count;
}

int update_task_state(SQLite sqlite,const char *task_id,int state){
    char sql_buff[4096] ={0};
    if(task_id == NULL || state > TASK_STATE_MAX || state < 0){
        return -1;
    }

    if(check_task(sqlite,task_id)){
        snprintf(sql_buff,sizeof(sql_buff),"update task set state = %d where task_id = '%s'",state,task_id);
        rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
        if(rc != SQLITE_OK){
            skerror("sqlite sql %s %s",sql_buff,zErr);
            sqlite3_free(zErr);
            zErr = NULL;
            return -1;
        }
        return 0;
    }else{
        return -1;
    }
}

int update_task_progress(SQLite *sqlite,const char *task_id,int percent){
    char sql_buff[4096] ={0};
    if(task_id == NULL || percent < 0){
        return -1;
    }
    if(check_task(sqlite,task_id)){
        snprintf(sql_buff,sizeof(sql_buff),"update task set progress = %d where task_id = '%s'",percent,task_id);
        rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
        if(rc != SQLITE_OK){
            skerror("sqlite sql %s %s",sql_buff,zErr);
            sqlite3_free(zErr);
            zErr = NULL;
            return -1;
        }
        return 0;
    }else{
        return -1;
    }
}



int delete_task(SQLite *sqlite,const char *task_id){
    char sql_buff[4096] ={0};

    if(task_id == NULL){
        return -1;
    }

    snprintf(sql_buff,sizeof(sql_buff),"delete from task where task_id = '%s';",task_id)

    rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}


int get_task_by_state(SQLite *sqlite,std::vector<task_info> &tasks,int state){
    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff), "select (task_id,host,path,data,port,task_state,percent,send_only,"
        "try_time,save_path,offset) from task where task_state =  %d ",state);
    sqlite3_stmt *pStmt = NULL;
    int rc = sqlitew_exec_sql(sqlite,sql_buff,&tasks,task_table_callback);
    if(rc != SQLITE_OK){
        skerror("query fail %s ",sql_buff);
        return -1;
    }else{
        return tasks.size();
    }
}


int get_all_task(SQLite *sqlite,std::vector<task_info> &tasks){
    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff), "select (task_id,host,path,data,port,task_state,percent,send_only,"
        "try_time,save_path,offset) from task");
    sqlite3_stmt *pStmt = NULL;
    int rc = sqlitew_exec_sql(sqlite,sql_buff,&tasks,task_table_callback);
    if(rc != SQLITE_OK){
        skerror("query fail %s ",sql_buff);
        return -1;
    }else{
        return tasks.size();
    }
}

int get_task_by_id(SQLite *sqlite,std::vector<task_info> &tasks,const char *task_id){
    char sql_buff[4096]={0};
    if(task_id == NULL){
        return -1;
    }
    snprintf(sql_buff,sizeof(sql_buff), "select (task_id,host,path,data,port,task_state,percent,send_only,"
        "try_time,save_path,offset) from task where task_id = '%s' ",task_id);
    sqlite3_stmt *pStmt = NULL;
    int rc = sqlitew_exec_sql(sqlite,sql_buff,&tasks,task_table_callback);
    if(rc != SQLITE_OK){
        skerror("query fail %s ",sql_buff);
        return -1;
    }else{
        return tasks.size();
    }
}

int get_idle_task(SQLite *sqlite,std::vector<task_info> &tasks){
    return get_task_by_state(sqlite,tasks,TASK_STATE_IDLE);
}

int get_fail_task(SQLite *sqlite,std::vector<task_info> &tasks){
    return get_task_by_state(sqlite,tasks,TASK_STATE_FAIL);
}

int get_done_task(SQLite *sqlite,std::vector<task_info> &tasks){
    return get_task_by_state(sqlite,tasks,TASK_STATE_DONE);
}
