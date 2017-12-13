#include"NetworkDatabase.h"
#include"RefBase.h"

//mutex for single instance
static Mutex gDatabaseMutex;
//database store path
static const char *gNetDBPath = "./NetworkDB.db";
//init static member of network database
sp<NetworkDatabase> mNetworkDatabase = NULL;


sp<NetworkDatabase>& NetworkDatabase::getInstance(){
    AutoMutex _l(gDatabaseMutex);
    if(mNetworkDatabase == NULL){
        mNetworkDatabase = new NetworkDatabase();
    }
    return mNetworkDatabase;
}

sp<SqliteWrapper>& getDBWrapper(){
    if(mNetworkDatabase == NULL){
        getInstance();
    }
    return getInstance()->mDBWrapper;
}


void NetworkDatabase::createTables(){
    //create network database tables
    //don't change items order of each create sql language
    const char* xDnsSQL= "create table if not exists xdns (host text,ip text,ip_type int default 0,"
                                "fetch_type int default 0,conn_profile int64 default -1);";
    const char *xTaskSQL= "create table if not exists xtask (task_id text,module_name text,url text,send_data blob,task_type int,"
                                "send_only int,save_file text,send_file text,retry_times int,conn_timeout int64,task_timeout int64,process int default 0);";
    const char *xProcSQL= "create table if not exists xproc (task_id text primary key,task_state int default 0,"
                                    "start_time int64 default 0,conn_time int64 default 0,complete int default 0,try_times int default 0)";
    mDBWrapper->createTable(xDnsSQL);
    mDBWrapper->createTable(xTaskSQL);
    mDBWrapper->createTable(xProcSQL);
}

NetworkDatabase::NetworkDatabase(){
    mDBWrapper = new SqliteWrapper(gNetDBPath);
    //open database
    mDBWrapper->open();
    //create table
    createTables();
}


NetworkDatabase::~NetworkDatabase(){

}



    const char* xDnsSQL= "create table if not exists xdns (host text,ip text,ip_type int default 0,"
                                "fetch_type int default 0,conn_profile int64 default -1);";

//insert socket address to database
int NetworkDatabase::xDnsInsert(const SocketAddress &sa){
    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),
            "insert into xdns(host,ip,ip_type,fetch_type,conn_profile) values (\'%s\',\'%s\',%d,%d,%d,%ld);",
            sa.getHostName(),sa.getIp(),sa.getType(),sa.getFetchType(),sa.getConnProf());
    int ret = mDBWrapper->insert(sql_buff);
    return ret;
}

//check host and ip whether is exist or not
int NetworkDatabase::xDnsExist(const SocketAddress &sa){
    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select count(*) from xdns where host = '%s' AND ip = '%s' ;",sa.getHostName(),sa.getIp());
    int count = mDBWrapper->count(sql_buff);
    return count > 0?count,0;
}


static int NetworkDatabase::xDnsVCallback(ColumnEntry *colEntries,int count ,void *pArgs){
    if(p == NULL){
        return -1;
    }
    Vector<SocketAddress> *saArray  = (Vector<SocketAddress> *)pArgs ;
    SocketAddress sa(colEntries.
    //add to vector
    ptr_dns->push_back(entry);
    return 1;
}


int get_hosts_by_ip_type(SQLite *sqlite,const char *ip,std::vector<network_dns> &ips){
    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select host,ip,dns_server,ip_type,dns_type,"
                                "fail_times,conn_profile from xdns where ip = '%s' order by conn_profile;",ip);

    int rc = sqlitew_exec_sql(sqlite,sql_buff,&ips,dns_callback);
    if(rc != SQLITE_OK){
        skerror("query fail %s ",sql_buff);
        return -1;
    }else{
        return ips.size();
    }
}

int get_ips_by_host(SQLite *sqlite,const char *host,std::vector<network_dns> &ips){

    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select host,ip,dns_server,ip_type,dns_type,"
                                    "fail_times,conn_profile from xdns where host = '%s' order by conn_profile;",host);

    int rc = sqlitew_exec_sql(sqlite,sql_buff,&ips,dns_callback);
    if(rc != SQLITE_OK){
        skerror("query fail %s ",sql_buff);
        return -1;
    }else{
        return ips.size();
    }

}

int get_all_hosts(SQLite *sqlite,std::vector<network_dns> &ips){

    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select host,ip,dns_server,ip_type,dns_type,"
                                    "fail_times,conn_profile from xdns order by conn_profile;"); 

    int rc = sqlitew_exec_sql(sqlite,sql_buff,&ips,dns_callback);
    if(rc != SQLITE_OK){
        skerror("query fail %s ",sql_buff);
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
    snprintf(sql_buff,sizeof(sql_buff),"update xdns set use_times = use_times +1 where host = '%s' and ip = '%s' ;",host,ip);
    char *zErr = NULL;
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
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

    snprintf(sql_buff,sizeof(sql_buff),"update xdns set conn_profile = %ld where host = '%s' and ip = '%s' ;",conn_profile,host,ip);
    char *zErr = NULL;
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
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

    snprintf(sql_buff,sizeof(sql_buff),"update xdns set fail_times = %d where host = '%s' and ip = '%s' ;",fail_times,host,ip);
    char *zErr = NULL;
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
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
    snprintf(sql_buff,sizeof(sql_buff),"update xdns set ip = '%s'where host = '%s' and ip = '%s' ;",new_ip,host,ip);
    char* zErr = NULL;
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
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

    snprintf(sql_buff,sizeof(sql_buff),"delete from xdns where host = %s and ip = %s ;", host, ip);
    char* zErr = NULL;
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
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

    snprintf(sql_buff,sizeof(sql_buff),"delete from xdns where fail_times/use_time < %f ;",percent);
    char* zErr = NULL;
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
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

    if(limit < 0 ){
        return -1;
    }

    snprintf(sql_buff,sizeof(sql_buff),"delete from xdns where conn_profile > %ld;",limit);

    char* zErr = NULL;
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}
