#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include<assert.h>
#include"sqlite_wrapper.h"
#include"sqlite_network_xhost.h"

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

static void networkdb_init(){
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

SQLite* get_networkdb(){
    if(network_db  == NULL){
        networkdb_init();
    }
    assert(network_db != NULL);
    return network_db;
}

int main(int argc,char **argv){
    insert_xhost_host(get_networkdb(),"baidu.com",80,HTTP_SERVER);
    insert_xhost_host(get_networkdb(),"ibaidu.com",80,HTTP_SERVER);
    delete_xhost_by_port(get_networkdb(),"baidu.com",80);
    delete_xhost_by_host(get_networkdb(),"ibaidu.com");

    insert_xhost_host(get_networkdb(),"baidu.com",80,HTTP_SERVER);
    insert_xhost_host(get_networkdb(),"baidu.com",80,HTTP_SERVER);
    insert_xhost_host(get_networkdb(),"baidu.com",80,HTTP_SERVER);
    insert_xhost_host(get_networkdb(),"baidu.com",80,HTTP_SERVER);
    insert_xhost_host(get_networkdb(),"ibaidu.com",80,HTTP_SERVER);
    insert_xhost_host(get_networkdb(),"ibaidu.com",80,HTTP_SERVER);
    insert_xhost_host(get_networkdb(),"ibaidu.com",80,HTTP_SERVER);
    insert_xhost_host(get_networkdb(),"ibaidu.com",80,HTTP_SERVER);

    std::vector<NetworkXHost> hosts ;

    //get_xhost_by_type(get_networkdb(),hosts,HTTP_SERVER);
    get_xhost_all_hosts(get_networkdb(),hosts);

    std::vector<NetworkXHost>::iterator begin = hosts.begin();
    std::vector<NetworkXHost>::iterator end = hosts.end();
    skinfo("\n");
    skinfo("\n");
    while (begin < end){
        skinfo("host %s port %d type %d \n",begin->host.c_str(),begin->port,begin->type);
        begin ++;
    }
    return 0;
}




