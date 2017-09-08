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
//1.table dns (host text,ip text,ip_type int,dnt_type) 
//2.table connect(ip text,conn_profile long,fail_times int,invalidate int)
//3.table task(host text,path text,data bobl,task_state int,percent int, send_only int,try_time int,save_path text,offset long)
static void create_networkdb_table(SQLite *sqlite){
    //don't change items order of create language
    const char *host_sql = "create table thost if not exists (host text,port int,type int);";
    const char *dns_sql = "create table dns if not exists (host text,ip text,dns_server text,ip_type int default 0,"
                                "dns_type int default 0,fail_times int default 0,conn_profile int64 default 0x0fffffff);";
    const char *task_sql = "create table task if not exists (id int primary key autoincrement,host text,path text,data blob,tast_state \
                                    int,percent int ,send_only int ,try_time int, save_path text,offset int);";

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

enum DNSQUERY_MASK{
    DNS_HOST_MASK = 1<<0,
    DNS_IP_MASK = 1<<1,
    DNS_SERVER_MASK = 1<<2,
    DNS_IPTYPE_MASK = 1<<3,
    DNS_DNSTYPE_MASK = 1<<4,
    DNS_FAILTIMES_MASK = 1<<5,
    DNS_CPROFILE_MASK = 1<<6,
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

    if(entry == NULL|| entry->ip.entry()){
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
int dns_check_host_exist(SQLite *sqlite,const string host,const string ip){
    sqlite3_stmt *pStmt = NULL;
    if(host.empty() || ip.empty()){
        skwarn("input value is invalidate");
        return -1;
    }

    std::string sql = "select count(*) where host =";
    sql += host;
    sql += "AND ip=";
    sql = sql + ip + ";";

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

int query_dns(SQLite *sqlite,network_dns *entry,DNSQUERY_MASK mask,std::vector<network_dns> &ips){
    std::string sql = "select (host,ip,dns_server,ip_type,dns_type,fail_times,conn_profile) where ";
    sqlite3_stmt *pStmt = NULL;
    if(entry == NULL){
        return -1;
    }

    if(mask & DNS_HOST_MASK &&!entry->host.empty()){
        sql = sql + " host = " + entry->host;
    }

    if(mask & DNS_IP_MASK && !entry->ip.enmpty()){
        sql = sql + " ip = " + entry->ip;
    }

    if(mask & DNS_IPTYPE_MASK && entry->ip_type >=0 ){
        sql = sql + " ip_type = " + entry->ip_type;
    }

    if(mask & DNS_DNSTYPE_MASK && entry->dns_type >=0 ){
        sql = sql + " dns_type = " + entry->dns_type;
    }
    sql += " ;";
    int rc = sqlitew_exec_sql(sqlite,sql.c_str(),&ips,dns_callback);
    if(rc != SQLITE_OK){
        skerror("query fail %s ",sql.c_str());
        return -1;
    }else{
        return ips.size();
    }
}


int update_dns_connprofile(SQLite *sqlite,const char *host,const char *ip,int64_t conn_profile){
    char sql_buff[4096] ={0};
    if(host == NULL || ip == NULL){
        return -1;
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
    if(host == NULL || ip == NULL){
        return -1;
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
    if(host == NULL || ip == NULL){
        return -1;
    }
    snprintf(sql_buff,sizeof(sql_buff),"delete from dns where host = %s and ip = %s ;", host, ip)
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

int insert_host(SQLite *sqlite,const char *host,int port,int type){
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

int delete_host(SQLite *sqlite,const char *host,int port){
    if(host == NULL){
        skwarn("host is error %s ",host);
        return -1;
    }
    char sql_buff[4096] ={0};
    snprintf(sql_buff,sizeof(sql_buff),"delete from thost where host='%s' AND port = %d ", host,port);
    rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}

//this is task table operate 
//insert update delete
//const char *task_sql = "create table task if not exists (id int primary key autoincrement,host text,path text,data blob,tast_state \
//                                   int,percent int ,send_only int ,try_time int, save_path text,offset int);";


