#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<assert.h>
#include<pthread.h>
#include"sqlite3.h"
#include"sqlite_wrapper.h"

static const int SQLITE_SOFT_HEAP_LIMIT = 8 * 1024 * 1024;
static const int SQLITE_BUSY_TIMEOUT_MS = 2500;
static const int SQLITE_ENABLE_TRACE = 1;
static const int SQLITE_ENABLE_PROFILE = 1;


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
    SQLite* sqlitedb= (SQLite*)data;
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
    int verboseLog = !!data;
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

    int verboseLog = 0;
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


SQLite* sqlitew_open(const char *dbname,int flags){
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
    err = sqlite3_busy_timeout(db, SQLITE_BUSY_TIMEOUT_MS);
    if (err != SQLITE_OK) {
        skerror("Could not set busy timeout");
        sqlite3_close(db);
        return NULL;
    }


    // Create wrapper object.
    SQLite *sqlitedb = (SQLite *)malloc(sizeof(SQLite));
    assert(sqlitedb != NULL);

    memset(sqlitedb,0,sizeof(SQLite));
    pthread_mutex_init(&sqlitedb->mutex,NULL);
    sqlitedb->db = db;
    sqlitedb->openFlags = open_flags;
    sqlitedb->path = strdup(dbname);
    sqlitedb->canceled = 0;

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
    SQLite *sdb =  *sqlitedb;

    skinfo("closing sqlitedb %p \n",sdb->db);
    if(sdb->path != NULL){
        free(sdb->path);
        sdb->path = NULL;
    }
    int err = sqlite3_close(sdb->db);
    if (err != SQLITE_OK) {
        // This can happen if sub-objects aren't closed first.  Make sure the caller knows.
        skerror("sqlite3_close(%p) failed: %d", sdb->db, err);
    }
    free(sdb);
    *sqlitedb = NULL;
}


//sql string should be encoded with utf8
sqlite3_stmt* sqlitew_prepare_sql(SQLite *sqlitedb,const char *sql){
    sqlite3_stmt* statement;
    if(sql == NULL || sqlitedb == NULL){
        return NULL;
    }

    int err = sqlite3_prepare_v2(sqlitedb->db, sql,strlen(sql), &statement, NULL);
    if(err !=SQLITE_OK ){
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
                if(xCallback(pArg,pStmt) < 0 ){
                    skwarn("sql abort by user \n");
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
int sqlitew_exec_sql(SQLite *sqlitedb,const char *sql,void *pArg,int (xCallback)(void *pArg,sqlite3_stmt *tStmt)){
    if(sqlitedb == NULL || sqlitedb->db == NULL || sql == NULL){
        return -1;
    }
    sqlite3 *db = sqlitedb->db;
    pthread_mutex_lock(&sqlitedb->mutex);
    sqlite3_stmt* pStmt = sqlitew_prepare_sql(sqlitedb,sql);
    sqlitew_exec_stmt(pArg,pStmt,xCallback);
    int rc = sqlite3_finalize(pStmt);
    if(rc != SQLITE_OK){
        skerror("exec sql %s fail",sql);
    }
    pthread_mutex_unlock(&sqlitedb->mutex);
    return rc;
}
