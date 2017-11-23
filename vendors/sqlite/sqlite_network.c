#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include<assert.h>
#include"sqlite_wrapper.h"
#include"sqlite_network_xhost.h"
#include"sqlite_network_xdns.h"

//create network database
static void create_networkdb_table(SQLite *sqlite){
    //don't change items order of create language
    const char *host_sql = "create table if not exists xhost  (host text,port int,type int);";
    const char *dns_sql = "create table if not exists xdns (host text,ip text,dns_server text,ip_type int default 0,"
                                "dns_type int default 0,fail_times int default 0,use_times int default 0,"
                                "conn_profile int64 default 0x0fffffff);";
    const char *task_sql = "create table if not exists xtask (task_id text,module_name text,"
                                    "host text,path text,data blob,port int,task_state int"
                                    "task_type int,percent int ,send_only int ,retry_times int, save_path text,"
                                    "src_path text,offset_start int64, offset_length int64,"
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

/*
int insert_host_ip(SQLite *sqlite,network_dns *entry);
int dns_host_exist(SQLite *sqlite,const char *host,const char *ip);
int get_hosts_by_ip_type(SQLite *sqlite,const char *ip,std::vector<network_dns> &ips);
int get_ips_by_host(SQLite *sqlite,const char *host,std::vector<network_dns> &ips);
int get_all_hosts(SQLite *sqlite,std::vector<network_dns> &ips);
int update_dns_usetimes(SQLite *sqlite,const char *host,const char *ip);
int update_dns_connprofile(SQLite *sqlite,const char *host,const char *ip,int64_t conn_profile);
int update_dns_failtimes(SQLite *sqlite,const char *host,const char *ip,int fail_times);
int update_dns_ip(SQLite *sqlite,const char *host,const char *ip,const char *new_ip);
int delete_dns_entries(SQLite *sqlite, network_dns *entries,const char *host,const char *ip);
int delete_dns_by_success(SQLite *sqlite,float percent);
int delete_dns_by_connprofile(SQLite *sqlite, int64_t limit);
*/

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

    return 0;
}




