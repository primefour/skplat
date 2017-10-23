
#define MAX_CONN_PROFILE 0x0fffffff
    //const char *dns_sql = "create table dns if not exists (host text,ip text,dns_server text,ip_type int default 0,"
                                //"dns_type int default 0,conn_profile int64 default 0x0fffff,fail_times int default 0);";
enum IPTYPE{
    IPTYPE_V4,
    IPTYPE_V6, 
};


enum DNSTYPE{
    DNSTYPE_RAW,
    DNSTYPE_HTTP,
    DNSTYPE_HTTPS 
};

struct network_dns{
    std::string host;
    std::string ip;
    std::string dns_server;
    int ip_type;
    int dns_type;
    int fail_times;
    int64_t conn_profile;
};


int insert_dns(SQLite *sqlite,network_dns *entry){
    char sql_buff[4096]={0};

    if(entry == NULL|| entry->ip.empty()){
        return -1;
    }

    if(entry->host.empty()){
        entry->host = entry->ip;
    }

    snprintf(sql_buff,sizeof(sql_buff),"insert into dns(host,ip,dns_server,ip_type,dns_type,fail_times,conn_profile)"
            "values ('%s','%s','%s',%d,%d,%d,%lld);",
            entry->host.c_str(), entry->ip.c_str(),
            !entry->dns_server.empty()?entry->dns_server.c_str(),"NULL",
            entry->ip_type >=0 ?entry->ip_type:IPTYPE_V4,
            entry->dns_type >=0 ?entry->dns_type:DNSTYPE_RAW,
            entry->fail_times >=0 ?entry->fail_times:0,
            entry->conn_profile >=0 ?entry->conn_profile:MAX_CONN_PROFILE);
    rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}

//(host text,ip text,ip_type int,dns_type int,dns_server text,count_down int);";
int dns_host_exist(SQLite *sqlite,const char *host,const char *ip){
    char sql_buff[4096]={0};
    sqlite3_stmt *pStmt = NULL;
    if(host == NULL || ip == NULL){
        skwarn("input value is invalidate");
        return -1;
    }

    snprintf(sql_buff,sizeof(sql_buff),"select count(*) from dns where host = '%s' AND ip = '%s' ;",host,ip);
    int rc = sqlite3_prepare_v2(sqlite->db,sql_buff, -1, &pStmt,NULL);

    if(rc!=SQLITE_OK || pStmt == NULL){
        skerror("prepare sql %s  fail ",sql_buff);
        return -1;
    }
    rc = sqlite3_step(pStmt);
    int count = 0;

    if(rc != SQLITE_ROW){
        skerror("execute sql  %s fail ",sql_buff);
    }else{
        count = sqlite3_column_int(pStmt,0);
    }
    sqlite3_finalize(pStmt);
    return count;
}


static int dns_callback(void*p,sqlite3_stmt *pStmt){
    std::vector<network_dns> *ptr_dns = (std::vector<network_dns>*)p;
    network_dns entry; 
    int i = 0;
    //host text
    entry->host = std::string(sqlite_column_text(pStmt,i++));
    //ip text
    entry->ip = std::string(sqlite_column_text(pStmt,i++));
    //dns_server text
    entry->dns_server = std::string(sqlite_column_text(pStmt,i++));
    //ip_type int
    entry->ip_type = sqlite_column_int(pStmt,i++);
    //dns_type int
    entry->dns_type = sqlite_column_int(pStmt,i++);
    //fail times int
    entry->fail_times = sqlite_column_int(pStmt,i++);
    //conn profile int64_t
    entry->conn_profile = sqlite_column_int64(pStmt,i++);
    //add to vector
    ptr_dns->push_back(entry);
}


int get_hosts_by_ip_type(SQLite *sqlite,const char *ip,std::vector<network_dns> &ips){
    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select (host,ip,dns_server,ip_type,dns_type,"
                                "fail_times,conn_profile) from dns where ip = '%s' order by conn_profile;",ip);
    int rc = sqlitew_exec_sql(sqlite,sql.c_str(),&ips,dns_callback);
    if(rc != SQLITE_OK){
        skerror("query fail %s ",sql.c_str());
        return -1;
    }else{
        return ips.size();
    }
}

int get_ips_by_host(SQLite *sqlite,const char *host,std::vector<network_dns> &ips){

    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select (host,ip,dns_server,ip_type,dns_type,"
                                    "fail_times,conn_profile) from dns where host = '%s' order by conn_profile;",host);

    int rc = sqlitew_exec_sql(sqlite,sql.c_str(),&ips,dns_callback);
    if(rc != SQLITE_OK){
        skerror("query fail %s ",sql.c_str());
        return -1;
    }else{
        return ips.size();
    }

}

int get_all_hosts(SQLite *sqlite,std::vector<network_dns> &ips){

    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select (host,ip,dns_server,ip_type,dns_type,"
                                    "fail_times,conn_profile) from dns order by conn_profile;"); 

    int rc = sqlitew_exec_sql(sqlite,sql.c_str(),&ips,dns_callback);
    if(rc != SQLITE_OK){
        skerror("query fail %s ",sql.c_str());
        return -1;
    }else{
        return ips.size();
    }

}

int update_dns_usetimes(SQLite *sqlite,const char *host,const char *ip){
    char sql_buff[4096] ={0};
    if(ip == NULL){
        return -1;
    }

    if(host == NULL){
        host = ip;
    }
    snprintf(sql_buff,sizeof(sql_buff),"update dns set use_times = use_times +1 where host = '%s' and ip = '%s' ;",host,ip);
    rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("update %s fail %s ",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}


int update_dns_connprofile(SQLite *sqlite,const char *host,const char *ip,int64_t conn_profile){
    char sql_buff[4096] ={0};

    if(ip == NULL){
        return -1;
    }

    if(host == NULL){
        host = ip;
    }

    snprintf(sql_buff,sizeof(sql_buff),"update dns set conn_profile = %d where host = '%s' and ip = '%s' ;",conn_profile,host,ip);
    rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("update %s fail %s ",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}


int update_dns_failtimes(SQLite *sqlite,const char *host,const char *ip,int fail_times){
    char sql_buff[4096] ={0};
    if(ip == NULL){
        return -1;
    }

    if(host == NULL){
        host = ip;
    }

    snprintf(sql_buff,sizeof(sql_buff),"update dns set fail_times = %d where host = '%s' and ip = '%s' ;",fail_times,host,ip);
    rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("update %s fail %s ",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}

int update_dns_ip(SQLite *sqlite,const char *host,const char *ip,const char *new_ip){
    char sql_buff[4096] ={0};
    if(host == NULL || ip == NULL || new_ip == NULL){
        return -1;
    }
    snprintf(sql_buff,sizeof(sql_buff),"update dns set ip = '%s'where host = '%s' and ip = '%s' ;",new_ip,host,ip);
    rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("update %s fail %s ",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
    }
    return 0;

}

int delete_dns_entries(SQLite *sqlite, network_dns *entries,const char *host,const char *ip){
    char sql_buff[4096] ={0};

    if(ip == NULL){
        return -1;
    }

    if(host == NULL){
        host = ip;
    }

    snprintf(sql_buff,sizeof(sql_buff),"delete from dns where host = %s and ip = %s ;", host, ip);
    rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}

int delete_dns_by_success(SQLite *sqlite,float percent){
    char sql_buff[4096] ={0};

    if(percent < 0 ){
        return -1;
    }

    snprintf(sql_buff,sizeof(sql_buff),"delete from dns where fail_times/use_time < %f ;",percent);
    rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}

int delete_dns_by_connprofile(SQLite *sqlite, int64_t limit){
    char sql_buff[4096] ={0};

    if(percent < 0 ){
        return -1;
    }

    snprintf(sql_buff,sizeof(sql_buff),"delete from dns where conn_profile > %lld;",limit);

    rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);

    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}




//host sql operate
//const char *host_sql = "create table host if not exists (host text,type int);";
//insert,delete
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

    const char *task_sql = "create table task if not exists (task_id text,host text,access_path text,\
                                    save_path text,src_path text,sends int64,send_count int64, \
                                    recvs int64,recv_count int64,port int ,send_only int,task_state int, \
                                    retry_times int,task_type int,conn_timeout int64,task_timeout int64, \
                                    start_time int64,end_time int64,send_data blob,recv_data blob);";




struct task_info{
    std::string task_id; //task name or id
    std::string host; //server
    std::string access_path; //client visit path

    //file send or download
    std::string save_path; //the path download data where to save
    //send file 
    std::string src_path; //send a file to server

    //send data 
    int64_t sends; //the data size of has send
    int64_t send_count; //the data size for send

    //recv data
    int64_t recvs; //data size of has recv
    int64_t recv_count; //the data size should recv

    short port; //server port
    bool send_only; 
    int task_state;
    int retry_time; //retry times
    int task_type;

    int64_t conn_timeout; //ms
    int64_t task_timeout; //ms
    int64_t start_time;
    int64_t end_time;
    AutoBuffer send_data; //send data; if not send file
    AutoBuffer recv_data; //recv data; if not save to file 
};

static int task_table_callback(void*p,sqlite3_stmt *pStmt){
    std::vector<task_info> *ptr_dns = (std::vector<task_info>*)p;
    task_info entry; 
    int i = 0;
    //task id 
    entry->task_id = std::string(sqlite_column_text(pStmt,i++));
    //host text
    entry->host = std::string(sqlite_column_text(pStmt,i++));
    //access path text
    entry->access_path = std::string(sqlite_column_text(pStmt,i++));

    //where recv file to save 
    entry->save_path = std::string(sqlite_column_text(pStmt,i++));
    //the path of file will send
    entry->src_path = std::string(sqlite_column_text(pStmt,i++));

    entry->sends = sqlite_column_int64(pStmt,i++);
    entry->send_count  = sqlite_column_int64(pStmt,i++);


    entry->recvs = sqlite_column_int64(pStmt,i++);
    entry->recv_count  = sqlite_column_int64(pStmt,i++);

    entry->port = short(sqlite_column_int(pStmt,i++));
    entry->send_only = sqlite_column_int(pStmt,i++);

    entry->task_state = sqlite_column_int(pStmt,i++);

    entry->retry_time= sqlite_column_int(pStmt,i++);
    entry->task_type= sqlite_column_int(pStmt,i++);

    entry->conn_timeout = sqlite_column_int64(pStmt,i++);
    entry->task_timeout = sqlite_column_int64(pStmt,i++);

    entry->start_time= sqlite_column_int64(pStmt,i++);
    entry->end_time= sqlite_column_int64(pStmt,i++);

    int len = sqlite3_column_bytes(pStmt,i);
    const char* pData= (const char *)sqlite3_column_blob(pState,i++);
    if(pData != NULL && len > 0){
        entry->send_data.Write(pData,len);
    }

    int len = sqlite3_column_bytes(pStmt,i);
    const char* pData= (const char *)sqlite3_column_blob(pState,i++);
    if(pData != NULL && len > 0){
        entry->recv_data.Write(pData,len);
    }

    //add to vector
    ptr_dns->push_back(entry);
}


int insert_task(SQLite sqlite, task_info * entry){
    char sql_buff[4096*2]={0};

    if(entry == NULL|| entry->task_id.empty() || entry->host.empty()){
        return -1;
    }

    snprintf(sql_buff,sizeof(sql_buff),"insert into task(task_id,host,access_path,save_path, \
        src_path,sends,send_count,recvs,recv_count,port,send_only,task_state, \
            retry_time,task_type,conn_timeout,task_timeout,start_time,end_time,\
            send_data,recv_data) values ('%s','%s','%s','%s', \
                '%s',%lld,%lld,%lld,%lld,%d,%d,%d, \
                %d,%d,%lld,%lld,%lld,%lld, \
                ?,?);",
            entry->task_id.c_str(),
            entry->host.c_str(),
            !entry->access_path.empty()?entry->access_path.c_str():"NULL",
            !entry->save_path.empty()?entry->save_path.c_str():"NULL",
            !entry->src_path.empty()?entry->src_path.c_str():"NULL",
            entry->sends, entry->send_count, entry->recvs, entry->recv_count,entry->port,
            entry->send_only,TASK_STATE_IDLE,
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
