#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include<assert.h>
#include"sqlite_wrapper.h"
#include"sqlite_network_xhost.h"
#include"sqlite_network_xdns.h"
#include"sqlite_network_xtask.h"
#include"sqlite_network_task_proc.h"

//create network database
static void create_networkdb_table(SQLite *sqlite){
    //don't change items order of create language
    const char *host_sql = "create table if not exists xhost  (host text,port int,type int);";
    const char *dns_sql = "create table if not exists xdns (host text,ip text,dns_server text,ip_type int default 0,"
                                "dns_type int default 0,fail_times int default 0,use_times int default 0,"
                                "conn_profile int64 default -1);";

    const char *task_info_sql = "create table if not exists xtask (task_id text,module_name text,"
                                    "host text,port int,path text,send_data blob,task_type int,send_only int,"
                                    "save_file text,send_file text,retry_times int,conn_timeout int64,task_timeout int64,process int default 0);";

    const char *task_proc_sql = "create table if not exists xtask_proc(task_id text,task_state int default 0,"
                                    "start_time int64 default 0,conn_time int64 default 0,complete int default 0,try_times int default 0)";

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
    rc = sqlite3_exec(sqlite->db,task_info_sql,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("create table %s %s \n",task_info_sql,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        sqlite->dbError = 1;
    }

    rc = sqlite3_exec(sqlite->db,task_proc_sql,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("create table %s %s \n",task_proc_sql,zErr);
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

    network_dns tmp_dns("baidu.com","192.168.1.1","localhost");
    network_dns tmp_dns2("6baidu.com","192.168.1.1","localhost",DNSTYPE_HTTP);
    network_dns tmp_dns3("4baidu.com","192.168.1.1",IPTYPE_V6,"localhost",DNSTYPE_HTTP);
    network_dns tmp_dns4("baidu.com","192.168.1.1","localhost");
    network_dns tmp_dns5("baidu.com","192.168.1.1","localhost");
    network_dns tmp_dns6("baidu.com","192.168.1.1","localhost");
    network_dns tmp_dns7("baidu.com","192.168.1.1","localhost");

    insert_host_ip(get_networkdb(),&tmp_dns);
    insert_host_ip(get_networkdb(),&tmp_dns);
    insert_host_ip(get_networkdb(),&tmp_dns);
    insert_host_ip(get_networkdb(),&tmp_dns);
    insert_host_ip(get_networkdb(),&tmp_dns2);
    insert_host_ip(get_networkdb(),&tmp_dns3);
    insert_host_ip(get_networkdb(),&tmp_dns4);
    skinfo("#############################################################\n");
    if(dns_host_exist(get_networkdb(),"baidu.com","192.168.1.1")){
        skinfo("tmp dns exist in the table \n");
    }else{
        skinfo("tmp dns don't exist in the table \n");
    }
    std::vector<network_dns> ips;
    get_hosts_by_ip_type(get_networkdb(),"192.168.1.1",ips);

    std::vector<network_dns>::iterator ibegin = ips.begin();
    std::vector<network_dns>::iterator iend = ips.end();

    while(ibegin < iend){
        skinfo("xxxx %s %s %s \n",ibegin->host.c_str(),ibegin->ip.c_str(),ibegin->dns_server.c_str());
        ibegin ++;
    }

    skinfo("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXtask test \n");
/*
 *
int xtask_insert_task(SQLite *sqlitedb, task_info *entry);
int xtask_todo_count(SQLite *sqlitedb);
int xtask_get_todo_tasks(SQLite * sqlitedb,std::vector<task_info> &tasks);
int xtask_delete_task(SQLite *sqlitedb,std::string task_id);
int xtask_update_process(SQLite * sqlitedb,std::string task_id,int process);
*/
    int count = xtask_todo_count(get_networkdb());
    skinfo("xtask count = %d \n",count);
    task_info entry("task1","module1","www.baidu.com","/");
    int ret = xtask_insert_task(get_networkdb(),&entry);
    skinfo("ret = %d \n",ret);
    count = xtask_todo_count(get_networkdb());
    skinfo("xtask count = %d \n",count);
    ret = xtask_delete_task(get_networkdb(),entry.task_id);
    count = xtask_todo_count(get_networkdb());
    skinfo("xtask count = %d \n",count);

    task_info entry2("task2","module1","www.baidu.com","/");
    task_info entry3("task3","module1","www.baidu.com","/");
    task_info entry4("task4","module1","www.baidu.com","/");
    task_info entry5("task5","module1","www.baidu.com","/");
    task_info entry6("task6","module1","www.baidu.com","/");
    task_info entry7("task17","module1","www.baidu.com","/");

    ret += xtask_insert_task(get_networkdb(),&entry);
    ret += xtask_insert_task(get_networkdb(),&entry2);
    ret += xtask_insert_task(get_networkdb(),&entry3);
    ret += xtask_insert_task(get_networkdb(),&entry4);
    ret += xtask_insert_task(get_networkdb(),&entry5);
    ret += xtask_insert_task(get_networkdb(),&entry6);
    ret += xtask_insert_task(get_networkdb(),&entry7);
    count = xtask_todo_count(get_networkdb());
    skinfo("xtask count = %d \n",count);
    std::vector<task_info> xtasks;
    ret = xtask_get_todo_tasks(get_networkdb(),xtasks);
    skinfo("ret = %d \n",ret);

    /*
struct task_info{
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
    int retry_time; //retry times
    int task_type;
    //profile and limit for network 
    long int conn_timeout; //ms
    long int task_timeout; //ms
};
*/
    return 0;
}




