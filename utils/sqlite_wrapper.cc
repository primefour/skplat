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

//create network database
//1.table dns (host text,ip text,ip_type int,dnt_type) 
//2.table connect(ip text,conn_profile long,fail_times int,invalidate int)
//3.table task(host text,path text,data bobl,task_state int,percent int, send_only int,try_time int,save_path text,offset long)
static void create_networkdb_table(SQLite *sqlite){
    const char *dns_sql = "create table dns if not exist (host text,ip text,ip_type int,dns_type int,dns_server text,count_down int);";
    const char *connect_sql = "create table connect if not exist(ip text,conn_profile int,fail_times int,invalid int);";
    const char *task_sql = "create table task(id int primary key autoincrement,host text,path text,data blob,tast_state \
                                    int,percent int ,send_only int ,try_time int, save_path text,offset int);";
    char *zErr = NULL;
    rc = sqlite3_exec(sqlite->db,dns_sql,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        utf8_printf(p->out, "/****** %s ******/\n", zErr);
        sqlite3_free(zErr);
        zErr = NULL;
    }

    rc = sqlite3_exec(sqlite->db,connect_sql,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        utf8_printf(p->out, "/****** %s ******/\n", zErr);
        sqlite3_free(zErr);
        zErr = NULL;
    }

    rc = sqlite3_exec(sqlite->db,task_sql,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        utf8_printf(p->out, "/****** %s ******/\n", zErr);
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

struct network_dns{
    char *host;
    char *host_ip;
    char *dsn_server;
    int ip_type;
    int dns_type;
};


int insert_dns_entries(network_dns *entries){
}
int update_dns_entries(network_dns *entries){

}
int delete_dns_entries(network_dns *entries){

}


struct network_conn{
    int fail_times;
    char *host_ip;
    int validate;
    long conn_time;
};

int insert_connect_entries(





