#include<stdio.h>
#include<sqlite3.h>
#include"sklog.h"

#ifdef SKLOG_TAG
#undef SKLOG_TAG
#define SKLOG_TAG "sqlite_log"
#endif

static const int SQLITE_SOFT_HEAP_LIMIT = 8 * 1024 * 1024;
static const int SQLITE_BUSY_TIMEOUT_MS = 2500;
static const int SQLITE_ENABLE_TRACE = 1;
static const int SQLITE_ENABLE_PROFILE = 1;

struct SQLite{
    sqlite3* const db;
    const int openFlags;
    std::string path;
    volatile bool canceled;
    SQLite(sqlite3* db, int openFlags, const std::string& path):
                db(db), openFlags(openFlags), path(path), canceled(false) { }
};


// Called each time a statement begins execution, when tracing is enabled.
static void trace_callback(void *data, const char *sql) {
    skinfo("sqlite sql : %s", sql);
}

// Called each time a statement finishes execution, when profiling is enabled.
static void profile_callback(void *data, const char *sql, sqlite3_uint64 tm) {
    skinfo("sqlite sql:%s took %0.3f ms",sql, tm * 0.000001f);
}

// Called after each SQLite VM instruction when cancelation is enabled.
static int cancel_callback(void* data) {
    SQLite* sqlitedb= static_cast<SQLite*>(data);
    return sqlitedb->canceled;
}


static void sqlitew_error_info(int errcode, const char* sqlite_msg,char *message ,int n) {
    const char *error_msg = NULL;
    error_msg = "SQLITE-ERROR";
    switch (errcode & 0xff) { /* mask off extended error code */
        case SQLITE_IOERR:
            error_msg = "SQLITE_IOERR";
            break;
        case SQLITE_CORRUPT:
        case SQLITE_NOTADB: // treat "unsupported file format" error as corruption also
            error_msg = "SQLITE_CORRUPT";
            break;
        case SQLITE_CONSTRAINT:
            error_msg = "SQLITE_CONSTRAINT";
            break;
        case SQLITE_ABORT:
            error_msg = "SQLITE_ABORT";
            break;
        case SQLITE_DONE:
            error_msg = "SQLITE_DONE";
            sqlite_msg = NULL; // SQLite error message is irrelevant in this case
            break;
        case SQLITE_FULL:
            error_msg = "SQLITE_FULL";
            break;
        case SQLITE_MISUSE:
            error_msg = "SQLITE_MISUSE";
            break;
        case SQLITE_PERM:
            error_msg = "SQLITE_PERM";
            break;
        case SQLITE_BUSY:
            error_msg = "SQLITE_BUSY";
            break;
        case SQLITE_LOCKED:
            error_msg = "SQLITE_LOCKED";
            break;
        case SQLITE_READONLY:
            error_msg = "SQLITE_READONLY";
            break;
        case SQLITE_CANTOPEN:
            error_msg = "SQLITE_CANTOPEN";
            break;
        case SQLITE_TOOBIG:
            error_msg = "SQLITE_TOOBIG";
            break;
        case SQLITE_RANGE:
            error_msg = "SQLITE_RANGE";
            break;
        case SQLITE_NOMEM:
            error_msg = "SQLITE_NOMEM";
            break;
        case SQLITE_MISMATCH:
            error_msg = "SQLITE_MISMATCH";
            break;
        case SQLITE_INTERRUPT:
            error_msg = "SQLITE_INTERRUPT";
            break;
        default:
            error_msg = "SQLiteError";
            break;
    }
    snprintf(message,n,"%s (code %d)%s",error_msg,errcode,sqlite_msg); 
}

// Called each time a message is logged.
static void sqlitew_log_callback(void* data, int err_code, const char* msg) {
    char log_buff[4096]={0};
    bool verboseLog = !!data;
    if(verboseLog){
        sqlitew_error_info(err_code,msg,log_buff,sizeof(log_buff));
        skinfo("%s",log_buff);
    }
}

void sqlitew_init() {
    // Enable multi-threaded mode.  In this mode, SQLite is safe to use by multiple
    // threads as long as no two threads use the same database connection at the same
    // time (which we guarantee in the SQLite database wrappers).
    sqlite3_config(SQLITE_CONFIG_MULTITHREAD);

    // Redirect SQLite log messages to the Android log.
#if 0
    bool verboseLog = android_util_Log_isVerboseLogEnabled(SQLITE_LOG_TAG);
#endif
    bool verboseLog = false;
    sqlite3_config(SQLITE_CONFIG_LOG, &sqlitew_log_callback, verboseLog ? (void*)1 : NULL);

    // The soft heap limit prevents the page cache allocations from growing
    // beyond the given limit, no matter what the max page cache sizes are
    // set to. The limit does not, as of 3.5.0, affect any other allocations.
    sqlite3_soft_heap_limit(SQLITE_SOFT_HEAP_LIMIT);
    // Initialize SQLite.
    sqlite3_initialize();
}

static int coll_localized( void *not_used, int nKey1, const void *pKey1, int nKey2, const void *pKey2){
    int rc, n;
    n = nKey1<nKey2 ? nKey1 : nKey2;
    rc = memcmp(pKey1, pKey2, n);
    if( rc==0 ){
        rc = nKey1 - nKey2;
    }
    return rc;
}


SQLite* sqlitew_open(char *dbname,int flags = -1){
    int open_flags = 0;

    if(flags == -1){
        open_flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    }else{
        open_flags = flags;
    }

    sqlite3* db;
    int err = sqlite3_open_v2(dbname, &db,open_flags, NULL);
    if (err != SQLITE_OK) {
        return NULL;
    }

    err = sqlite3_create_collation(db, "localized", SQLITE_UTF8, 0, coll_localized);
    if (err != SQLITE_OK) {
        sqlite3_close(db);
        return NULL;
    }

    // Check that the database is really read/write when that is what we asked for.
    if ((open_flags & SQLITE_OPEN_READWRITE) && sqlite3_db_readonly(db, NULL)) {
        skerror("Could not open the database in read/write mode.");
        sqlite3_close(db);
        return NULL;
    }

    // Set the default busy handler to retry automatically before returning SQLITE_BUSY.
    err = sqlite3_busy_timeout(db, BUSY_TIMEOUT_MS);
    if (err != SQLITE_OK) {
        skerro("Could not set busy timeout");
        sqlite3_close(db);
        return NULL;
    }

    // Create wrapper object.
    Sqlite* sqlitedb= new Sqlite(db, open_flags, path);

    // Enable tracing and profiling if requested.
    if (SQLITE_ENABLE_TRACE) {
        sqlite3_trace(db, &trace_callback, sqlitedb);
    }
    if (SQLITE_ENABLE_PROFILE) {
        sqlite3_profile(db, &profile_callback,sqlitedb); 
    }

    skinfo("Opened connection %p with dbname: %s", db,dbname); 
    return sqlitedb;
}


void sqlitew_close(SQLite **sqlitedb) {
    if(*sqlitedb == NULL){
        return ;
    }

    ALOGV("Closing connection %p", (*sqlite)->db);
    int err = sqlite3_close((*sqlitedb)->db);
    if (err != SQLITE_OK) {
        // This can happen if sub-objects aren't closed first.  Make sure the caller knows.
        skerror("sqlite3_close(%p) failed: %d", (*sqlitedb)->db, err);
        return;
    }
    delete *sqlitedb;
    *sqlitedb = NULL;
}


//sql string should be encoded with utf8
sqlite3_stmt* sqlitew_prepare_sql(SQLite *sqlitedb,const char *sql){
    sqlite3_stmt* statement;
    if(sql == NULL || sqlitedb == NULL){
        return NULL;
    }

    int err = sqlite3_prepare_v2(sqlitedb->db, sql,strlen(sql), &statement, NULL);
    if( rc !=SQLITE_OK ){
        skerror("sqlite prepare fail (%s) failed: %d",sql,err);
        return NULL;
    }
    return statement;
}

/*
** Run a prepared statement
*/
void sqlitew_exec_stmt(void *pArg, sqlite3_stmt *pStmt, /* Statment to run */ 
                    int (*xCallback)(void *pArg,sqlite3_stmt *tStmt)){
    int rc;
    /* perform the first step.  this will tell us if we
     ** have a result set or not and how wide it is.
     */
    rc = sqlite3_step(pStmt);
    /* if we have a result set... */
    if( SQLITE_ROW == rc ){
        /* if we have a callback... */
        if( xCallback ){
            /* allocate space for col name ptr, value ptr, and type */
            /* if data and types extracted successfully... */
            do{
                /* call the supplied callback with the result row data */
                if(xCallback(pArg,pStmtp)){
                    rc = SQLITE_ABORT;
                }else{
                    rc = sqlite3_step(pStmt);
                }
            } while( SQLITE_ROW == rc );
        }else{
            do{
                rc = sqlite3_step(pStmt);
            } while( rc == SQLITE_ROW );
        }
    }
}


/*
**Run a sql statement
*/

void sqlitew_exec_sql(SQLite *sqlitedb,const char *sql,void *pArg,int (xCallback)(void *pArg,sqlite3_stmt *tStmt)){
    if(sqlitedb == NULL || sqlitedb->db == NULL || sql == NULL){
        return ;
    }

    sqlite3 db = sqlitedb->db;

    if( !sqlite3SafetyCheckOk(db) ){
        skerror("sqlite fail SQLITE_MISUSE_BKPT");
        return ;
    }

    sqlite3_mutex_enter(db->mutex);
    sqlite3Error(db, SQLITE_OK);
    sqlite3_stmt* pStmt = sqlitew_prepare_sql(sqlitedb,sql);
    sqlitew_exec_stmt(pArg,pStmt,xCallback);
    int rc = sqlite3_finalize(pStmt);
    if(rc != SQLITE_OK){
        skerror("exec sql %s fail",sql);
    }
    sqlite3_mutex_leave(db->mutex);
}


//create network database
static void create_networkdb_table(SQLite *sqlite){
    //don't change items order of create language
    const char *host_sql = "create table thost if not exists (host text,port int,type int);";
    const char *dns_sql = "create table dns if not exists (host text,ip text,dns_server text,ip_type int default 0,"
                                "dns_type int default 0,fail_times int default 0,use_times int default 0,"
                                "conn_profile int64 default 0x0fffffff);";

    const char *task_sql = "create table task if not exists (task_id text,host text,path text,data blob,port int,task_state \
                                    int,percent int ,send_only int ,try_time int, save_path text,offset int, \
                                    start_time int64,conn_time int64,time_out int64);";

    char *zErr = NULL;
    rc = sqlite3_exec(sqlite->db,host_sql,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("create table %s %s ",host_sql,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
    }

    char *zErr = NULL;
    rc = sqlite3_exec(sqlite->db,dns_sql,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("create table %s %s",dns_sql,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
    }
    rc = sqlite3_exec(sqlite->db,task_sql,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("create table %s %s ",task_sql,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
    }
}

SQLite* gsqlitedb = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INIT;

void networkdb_init(){
    const char *dbname = "/sdcard/netwr.db";
    pthread_mutex_lock(&mutex);
    if(gsqlitedb != NULL){
        pthread_mutex_unlock(&mutex);
        return;
    }
    sqlitew_init();
    SQLite* gsqlitedb = sqlitew_open(dbname);
    if (gsqlitedb == NULL){
        skerror("create db %s fail ",dbname);
        pthread_mutex_unlock(&mutex);
        return;
    }
    create_networkdb_table(sqlitedb);
    pthread_mutex_unlock(&mutex);
}


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
enum HOST_TYPE{
    HTTP_SERVER,
    HTTP_DNS_SERVER,
    HTTP_DNS_SERVER,
    HTTPS_SERVER,
    DNS_SERVER,
    TCP_SERVER,
    TLS_SERVER,
    HOST_TYPE_MAX,
};

struct network_host{
    std::string host;
    int port;
    int type;
};

static int thost_callback(void*p,sqlite3_stmt *pStmt){
    std::vector<network_host> *ptr_thost = (std::vector<network_host>*)p;
    network_host entry; 
    int i = 0;
    //host name or ip 
    entry->host = std::string(sqlite_column_text(pStmt,i++));
    //host listen port 
    entry->port = sqlite_column_int(pStmt,i++);
    //host type 
    entry->type = sqlite_column_int(pStmt,i++);
    //add to vector
    ptr_thost->push_back(entry);
}

int get_thost_all_hosts(SQLite *sqlite,std::vector<network_host> &ips){
    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select (host,port,type) from thost ;");
    int rc = sqlitew_exec_sql(sqlite,sql.c_str(),&ips,thost_callback);
    if(rc != SQLITE_OK){
        skerror("query fail %s ",sql_buff);
        return -1;
    }else{
        return ips.size();
    }

}

int get_thost_by_type(SQLite *sqlite,std::vector<network_host> &ips,int type ){
    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select (host,port,type) from thost where type = %d;",type);
    int rc = sqlitew_exec_sql(sqlite,sql.c_str(),&ips,thost_callback);
    if(rc != SQLITE_OK){
        skerror("query fail %s ",sql_buff);
        return -1;
    }else{
        return ips.size();
    }
}


int insert_thost_host(SQLite *sqlite,const char *host,int port,int type){
    if(type >= HOST_TYPE_MAX || type < 0){
        skwarn("host type is error %d ",type);
        return -1;
    }
    if(host == NULL){
        skwarn("host is error %s ",host);
        return -1;
    }
    char sql_buff[4096] ={0};
    snprintf(sql_buff,sizeof(sql_buff),"insert into thost(host,port,type)values('%s',%d,%d);", host,port,type);
    rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}

int delete_thost_by_port(SQLite *sqlite,const char *host,int port){
    if(host == NULL){
        skwarn("host is error %s ",host);
        return -1;
    }
    char sql_buff[4096] ={0};
    snprintf(sql_buff,sizeof(sql_buff),"delete from thost where host='%s' AND port = %d;", host,port);
    rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}

int delete_thost_by_host(SQLite *sqlite,const char *host){
    if(host == NULL){
        skwarn("host is error %s ",host);
        return -1;
    }
    char sql_buff[4096] ={0};
    snprintf(sql_buff,sizeof(sql_buff),"delete from thost where host='%s';", host,port);
    rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}

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


