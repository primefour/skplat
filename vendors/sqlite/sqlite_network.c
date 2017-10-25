#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include<assert.h>
#include"sqlite_wrapper.h"

//create network database
static void create_networkdb_table(SQLite *sqlite){
    //don't change items order of create language
    const char *host_sql = "create table if not exists xhost  (host text,port int,type int);";
    const char *dns_sql = "create table if not exists xdns (host text,ip text,dns_server text,ip_type int default 0,"
                                "dns_type int default 0,fail_times int default 0,use_times int default 0,"
                                "conn_profile int64 default 0x0fffffff);";
    const char *task_sql = "create table if not exists xtask (task_id text,host text,path text,data blob,port int,task_state "
                                    "int,percent int ,send_only int ,retry_times int, save_path text,offset_start int64, offset_length int64,"
                                    "start_time int64,conn_time int64,conn_timeout int64,task_timeout int64);";
    char *zErr = NULL;
    int rc = sqlite3_exec(sqlite->db,host_sql,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("create table %s %s \n",host_sql,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        sqlite->dbError = 1;
    }

    rc = sqlite3_exec(sqlite->db,dns_sql,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("create table %s %s \n",dns_sql,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        sqlite->dbError = 1;
    }
    rc = sqlite3_exec(sqlite->db,task_sql,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("create table %s %s \n",task_sql,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        sqlite->dbError = 1;
    }
}

SQLite *network_db = NULL;
pthread_mutex_t network_mutex = PTHREAD_MUTEX_INITIALIZER;

SQLite* get_networkdb(){
    assert(network_db != NULL);
    if(network_db  == NULL){
        skerror("Please call network init before calling this function\n"); 
    }
    return network_db;
}


void networkdb_init(){
    const char *dbname = "./netwr.db";
    pthread_mutex_lock(&network_mutex );
    if(network_db != NULL){
        pthread_mutex_unlock(&network_mutex );
        return;
    }
    sqlitew_init();
    network_db = sqlitew_open(dbname,-1);
    if(network_db == NULL){
        skerror("create db %s fail ",dbname);
        pthread_mutex_unlock(&network_mutex );
        return;
    }
    create_networkdb_table(network_db);
    pthread_mutex_unlock(&network_mutex );
}
