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
    snprintf(sql_buff,sizeof(sql_buff),
            "select count(*) from xdns where host = '%s' AND ip = '%s' ;",sa.getHostName(),sa.getIp());
    int count = mDBWrapper->count(sql_buff);
    return count > 0?count,0;
}

/*
 *(host text,ip text,ip_type int default 0,"
 *"fetch_type int default 0,conn_profile int64 default -1);";
 */
static int NetworkDatabase::xDnsVCallback(KeyedHash<std::string,ColumnEntry> *colEntries,void *pArgs){
    static std::string hostKey("host");
    static std::string ipKey("ip");
    static std::string ipTypeKey("ip_type");
    static std::string fetchTypeKey("fetch_type");
    static std::string profKey("conn_profile");
    if(colEntries  == NULL || pArgs == NULL){
        ALOGW("%s arguments is NULL",__func__);
        return -1;
    }
    //get array of socket address
    Vector<SocketAddress> *saArray  = (Vector<SocketAddress> *)pArgs ;
    //create socket address entry
    SocketAddress sa(colEntries.get(hostKey).getString(),colEntries.get(ipKey).getString());
    sa.setFetchType(colEntries.get(fetchTypeKey).getLong());
    sa.setConnProf(colEntries.get(profKey).getLong());
    //insert to vector
    saArray->push(sa);
    return 1;
}

int NetworkDatabase::getAddrByHost(const char *host,Vector<SocketAddress> &ips){
    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select host,ip,ip_type,fetch_type,"
                                    "conn_profile from xdns where host = '%s' order by conn_profile;",host);

    int rc = mDBWrapper->execSql(sql_buff,xDnsVCallback,&ips);
    if(rc != OK){
        skerror("query fail %s ",sql_buff);
        return UNKNOWN_ERROR;
    }else{
        return ips.size();
    }

}

int NetworkDatabase::xDnsUpdateProf(const char *host,const char *ip,int64_t conn_profile){
    char sql_buff[4096] ={0};
    if(ip == NULL){
        return -1;
    }

    if(host == NULL){
        host = ip;
    }

    snprintf(sql_buff,sizeof(sql_buff),
            "update xdns set conn_profile = %ld where host = '%s' and ip = '%s' ;",conn_profile,host,ip);
    int rc = mDBWrapper->execSql(sql_buff,(xCallback)NULL,NULL);
    return rc;
}

int NetworkDatabase::xDnsUpdateHostIp(SQLite *sqlite,const char *host,const char *ip,const char *new_ip){
    char sql_buff[4096] ={0};
    if(host == NULL || ip == NULL || new_ip == NULL){
        return UNKNOWN_ERROR;
    }
    snprintf(sql_buff,sizeof(sql_buff),
            "update xdns set ip = '%s',conn_profile = -1 where host = '%s' and ip = '%s' ;",new_ip,host,ip);
    int rc = mDBWrapper->execSql(sql_buff,(xCallback)NULL,NULL);
    return rc;

}

int NetworkDatabase::xDnsDelete(const char *host,const char *ip){
    char sql_buff[4096] ={0};
    if(ip == NULL){
        return -1;
    }
    if(host == NULL){
        host = ip;
    }
    snprintf(sql_buff,sizeof(sql_buff),"delete from xdns where host = %s and ip = %s ;", host, ip);
    int rc = mDBWrapper->execSql(sql_buff,(xCallback)NULL,NULL);
    return rc;
}

