#include"Log.h"
#include"SqliteWrapper.h"
#include"Mutex.h"
#include<stdio.h>
#include"KeyedHash.h"

static const int SQLITE_SOFT_HEAP_LIMIT = 8 * 1024 * 1024;
static const int SQLITE_BUSY_TIMEOUT_MS = 2500;
static const int SQLITE_ENABLE_TRACE = 1;
static const int SQLITE_ENABLE_PROFILE = 1;

bool SqliteWrapper::mVerboseLog = true;

template<> ColumnEntry KeyedHash<std::string ,ColumnEntry >::mInvalidate  = ColumnEntry();

SqliteWrapper::SqliteWrapper(std::string path):mDatabasePath(path){
    ALOGD("database create");
    mCanceled = 0;
    mDatabaseError = 0;
    mOpenFlags |= SQLITE_OPEN_READWRITE;
}

SqliteWrapper::~SqliteWrapper(){
    ALOGD("database destory ");
    dispose();
}


int SqliteWrapper::createTable(const char *sql){
    int rc = execSql(sql,(vCallback)NULL,NULL);
    if(rc != OK){
        ALOGW("create table fail %s ",sql);
    }
    return rc;
}

int SqliteWrapper::insert(const char *sql){
    int rc = execSql(sql,(vCallback)NULL,NULL);
    if(rc != OK){
        ALOGW("insert value fail %s ",sql);
    }
    return rc;
}

int SqliteWrapper::update(const char *sql){
    int rc = execSql(sql,(vCallback)NULL,NULL);
    if(rc != OK){
        ALOGW("update value fail %s ",sql);
    }
    return rc;
}

int SqliteWrapper::xdelete(const char *sql){
    int rc = execSql(sql,(vCallback)NULL,NULL);
    if(rc != OK){
        ALOGW("delete value fail %s ",sql);
    }
    return rc;
}

int SqliteWrapper::query(const char *sql,vCallback cb,void *pArgs){
    int rc = execSql(sql,cb,pArgs);
    if(rc != OK){
        ALOGW("query value fail %s ",sql);
    }
    return rc;
}

int SqliteWrapper::query(const char *sql,xCallback cb,void *pArgs){
    int rc = execSql(sql,cb,pArgs);
    if(rc != OK){
        ALOGW("query value fail %s ",sql);
    }
    return rc;
}

int SqliteWrapper::countCallback(sqlite3_stmt *pStmt,void *pArg){
    if(pStmt == NULL || pArg == NULL){
        ALOGW("countCallback parameter is NULL");
        return 0;
    }
    int *count = (int *)pArg;
    *count = sqlite3_column_int(pStmt,0);
    ALOGD("%s ==> %d ",__func__,*count);
    return *count;
}

int SqliteWrapper::count(const char *sql){
    int tmpCount= 0;
    int rc = execSql(sql,countCallback,&tmpCount);
    if(rc != OK){
        ALOGW("count execute fail %s ",sql);
        return UNKNOWN_ERROR;
    }
    return tmpCount;
}


int SqliteWrapper::open(int flags){
    AutoMutex _l(mMutex);
    if(mDatabase != NULL){
        return OK;
    }
    // Enable multi-threaded mode.  In this mode, SQLite is safe to use by multiple
    // threads as long as no two threads use the same database connection at the same
    // time (which we guarantee in the SQLite database wrappers).
    sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
    int verboseLog = 0;
    sqlite3_config(SQLITE_CONFIG_LOG, &logCallback, mVerboseLog? (void*)1 : NULL);
    // The soft heap limit prevents the page cache allocations from growing
    // beyond the given limit, no matter what the max page cache sizes are
    // set to. The limit does not, as of 3.5.0, affect any other allocations.
    sqlite3_soft_heap_limit(SQLITE_SOFT_HEAP_LIMIT);
    // Initialize SQLite.
    sqlite3_initialize();

    int openFlags = 0;
    if(flags == -1){
        openFlags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    }else{
        openFlags = flags;
    }
    mOpenFlags = openFlags;

    int err = sqlite3_open_v2(mDatabasePath.c_str(), &mDatabase,mOpenFlags,NULL);
    if (err != SQLITE_OK) {
        ALOGE("open database %s fail",mDatabasePath.c_str());
        return UNKNOWN_ERROR;
    }

    err = sqlite3_create_collation(mDatabase,"localized", SQLITE_UTF8, 0, collLocalized);

    if (err != SQLITE_OK) {
        ALOGE("set localized fail %s fail",mDatabasePath.c_str());
        dispose();
        return UNKNOWN_ERROR;
    }

    // Check that the database is really read/write when that is what we asked for.
    if ((mOpenFlags & SQLITE_OPEN_READWRITE) && sqlite3_db_readonly(mDatabase, NULL)) {
        ALOGE("Could not open the database in read/write mode.");
        dispose();
        return UNKNOWN_ERROR;
    }

    // Set the default busy handler to retry automatically before returning SQLITE_BUSY.
    err = sqlite3_busy_timeout(mDatabase, SQLITE_BUSY_TIMEOUT_MS);
    if (err != SQLITE_OK) {
        ALOGE("Could not set busy timeout");
        dispose();
        return UNKNOWN_ERROR;
    }

    // Enable tracing and profiling if requested.
    if (SQLITE_ENABLE_TRACE) {
        sqlite3_trace(mDatabase, &traceCallback,this);
    }

    if (SQLITE_ENABLE_PROFILE) {
        sqlite3_profile(mDatabase, &profileCallback,this); 
    }
    ALOGD("Opened connection db %p object %p  with dbname: %s",mDatabase,this,mDatabasePath.c_str()); 
    return OK;
}

void SqliteWrapper::dispose(){
    if(mDatabase != NULL){
        int ret = sqlite3_close(mDatabase);
        if (ret != SQLITE_OK) {
            ErrorMsg();
            // This can happen if sub-objects aren't closed first.  Make sure the caller knows.
            ALOGD("close database fail db  %p object %p with dbname %s ",mDatabase,this,mDatabasePath.c_str());
        }else{
            ALOGD("close database db %p object %p with dbname %s ",mDatabase,this,mDatabasePath.c_str());
        }
        mDatabase = NULL;
    }
}

/*
 * call prepare to compile raw sql
 */
sqlite3_stmt* SqliteWrapper::compileSQL(const char *sql){
    sqlite3_stmt* statement;
    if(sql == NULL || mDatabase == NULL){
        return NULL;
    }
    int err = sqlite3_prepare_v2(mDatabase,sql,strlen(sql), &statement, NULL);
    if(err !=SQLITE_OK ){
        errorInc();
        ALOGE("sqlite prepare fail (%s) failed: %d",sql,err);
        return NULL;
    }
    ALOGD("create a statment %p ",statement);
    return statement;
}

int SqliteWrapper::finalize(sqlite3_stmt* statement){
    //release statement
    int rc = sqlite3_finalize(statement);
    if(rc != SQLITE_OK){
        ALOGE("statment :%p failed finalize error msg:%s",statement,sqlite3_errmsg(mDatabase));
        errorInc();
        return UNKNOWN_ERROR;
    }
    ALOGD("finalize a statment %p rc =%d ",statement,rc);
    return OK;
}

void SqliteWrapper::getRowData(sqlite3_stmt *pStmt,int nCol,KeyedHash<std::string,ColumnEntry> *colEntries){
    ColumnEntry value;
    for (int i = 0; i < nCol; i++) {
        value.Name = (const char *)sqlite3_column_name(pStmt, i);
        ASSERT(!value.Name.empty(),"KEY IS EMPTY");
        int type = sqlite3_column_type(pStmt, i);
        if (type == SQLITE_TEXT) {
            // TEXT data
            value.Value.charValue = reinterpret_cast<const char*>(sqlite3_column_text(pStmt, i)); 
            value.Type = SQLITE_TEXT;
            value.Length = sqlite3_column_bytes(pStmt, i) +1;
            ALOGD("key:%s ==> %s ",value.Name.c_str(),value.Value.charValue);
        } else if (type == SQLITE_INTEGER) {
            // INTEGER data
            value.Value.longValue = sqlite3_column_int64(pStmt, i);
            value.Type = SQLITE_INTEGER;
            ALOGD("key:%s ==> %ld ",value.Name.c_str(),value.Value.longValue);
        } else if (type == SQLITE_FLOAT) {
            // FLOAT data
            value.Value.floatValue = sqlite3_column_double(pStmt, i);
            value.Type = SQLITE_FLOAT;
            ALOGD("key:%s ==> %f ",value.Name.c_str(),value.Value.floatValue);
        } else if (type == SQLITE_BLOB) {
            // BLOB data
            value.Value.charValue = (const char *)sqlite3_column_blob(pStmt, i);
            value.Length = sqlite3_column_bytes(pStmt, i);
            value.Type = SQLITE_BLOB;
            ALOGD("key:%s ==> %s ",value.Name.c_str(),value.Value.charValue);
        } else if (type == SQLITE_NULL) {
            value.Value.charValue = NULL; 
            value.Type = SQLITE_NULL;
            ALOGD("key:%s ==> %s ",value.Name.c_str(),value.Value.charValue);
        } else {
            ALOGW("get a invalidate data type of sqlite type %d",type);
            continue;
        }
        colEntries->add(value.Name,value);
    }
}

int SqliteWrapper::execStmt(void *pArg,sqlite3_stmt *pStmt,vCallback cb){
    int nCol = sqlite3_column_count(pStmt);
    KeyedHash<std::string,ColumnEntry> *colEntries = NULL;
    while(1){
        int i;
        int rc = sqlite3_step(pStmt);
        if(rc == SQLITE_ROW){
            //get a row data and now get column name
            if(colEntries == NULL && nCol > 0){
                colEntries = new KeyedHash<std::string,ColumnEntry>(nCol,getStringHash);
                if(colEntries == NULL){
                    ALOGW("execute statement fail  %p no memory ",pStmt);
                    return NO_MEMORY;
                }
            }
            getRowData(pStmt,nCol,colEntries);
            if(cb(colEntries,pArg) <= 0|| mCanceled ){
                rc = SQLITE_ABORT;
                ALOGW("statement %p abort by user",pStmt);
                mCanceled = false;
                if(colEntries != NULL){
                    delete colEntries;
                    colEntries = NULL;
                }
                return rc;
            }
        }else if(rc == SQLITE_DONE){
            if(colEntries != NULL){
                delete colEntries;
                colEntries = NULL;
            }
            //sql execute done
            return OK;
        }
    }
    if(colEntries != NULL){
        delete colEntries;
        colEntries = NULL;
    }
    return UNKNOWN_ERROR; 
}

/*
** Run a prepared statement
*/
int SqliteWrapper::execStmt(void *pArg,sqlite3_stmt *pStmt,xCallback cb){
    int rc = 0;
    rc = sqlite3_step(pStmt);
    /* if we have a result set... */
    if(SQLITE_ROW == rc ){
        /* if we have a callback... */
        if(cb){
            /* allocate space for col name ptr, value ptr, and type */
            /* if data and types extracted successfully... */
            do{
                /* call the supplied callback with the result row data */
                if(cb(pStmt,pArg) <= 0 || mCanceled){
                    ALOGW("statement %p abort by user",pStmt);
                    rc = SQLITE_ABORT;
                    mCanceled = false;
                    return rc;
                }else{
                    rc = sqlite3_step(pStmt);
                }
            } while(SQLITE_ROW == rc );
        }else{
            do{
                rc = sqlite3_step(pStmt);
            } while(rc == SQLITE_ROW );
        }
    }

    if(rc == SQLITE_DONE){
        return OK;
    }else{
        errorInc();
        ALOGE("ERROR EXECUTE STATEMENT %p ",pStmt);
        return UNKNOWN_ERROR;
    }
}

int SqliteWrapper::execSql(const char *sql,vCallback cb,void *arg){
    AutoMutex _l(mMutex);
    if(mDatabase == NULL || sql == NULL){
        ALOGW("INVALID_OPERATION execsql");
        return INVALID_OPERATION;
    }
    //compile sql
    sqlite3_stmt* pStmt = compileSQL(sql);
    if(pStmt == NULL){
        return UNKNOWN_ERROR;
    }
    //execute statement
    execStmt(arg,pStmt,cb);
    //release statement
    return finalize(pStmt);
}

int SqliteWrapper::execSql(const char *sql,xCallback cb,void *arg){
    AutoMutex _l(mMutex);
    if(mDatabase == NULL || sql == NULL){
        ALOGW("INVALID_OPERATION execsql");
        return INVALID_OPERATION;
    }
    //compile sql
    sqlite3_stmt* pStmt = compileSQL(sql);
    if(pStmt == NULL){
        return UNKNOWN_ERROR;
    }
    //execute statement
    execStmt(arg,pStmt,cb);
    //release statement
    return finalize(pStmt);
}

void SqliteWrapper::traceCallback(void *data,const char *sql){
    ALOGD("database %p execute sql: %s ",data,sql);
}

void SqliteWrapper::profileCallback(void *data,const char *sql,sqlite3_uint64 tm){
    ALOGD("database %p execute sql:%s took %0.3f ms",data,sql,tm * 0.000001f);
}

void SqliteWrapper::logCallback(void* data, int errCode, const char* msg){
    if(mVerboseLog){
        std::string logMsg;
        errString(errCode,msg,logMsg);
        ALOGI("call log msg %s ",logMsg.c_str());
    }
}

void SqliteWrapper::errString(int errCode,const char*sqliteMsg,std::string &errMsg){
    const char *codeInfo = NULL;
    codeInfo = "SQLITE-ERROR";
    switch (errCode& 0xff) { /* mask off extended error code */
        case SQLITE_IOERR:
            codeInfo = "SQLITE_IOERR";
            break;
        case SQLITE_CORRUPT:
        case SQLITE_NOTADB: // treat "unsupported file format" error as corruption also
            codeInfo = "SQLITE_CORRUPT";
            break;
        case SQLITE_CONSTRAINT:
            codeInfo = "SQLITE_CONSTRAINT";
            break;
        case SQLITE_ABORT:
            codeInfo = "SQLITE_ABORT";
            break;
        case SQLITE_DONE:
            codeInfo = "SQLITE_DONE";
            break;
        case SQLITE_FULL:
            codeInfo = "SQLITE_FULL";
            break;
        case SQLITE_MISUSE:
            codeInfo = "SQLITE_MISUSE";
            break;
        case SQLITE_PERM:
            codeInfo = "SQLITE_PERM";
            break;
        case SQLITE_BUSY:
            codeInfo = "SQLITE_BUSY";
            break;
        case SQLITE_LOCKED:
            codeInfo = "SQLITE_LOCKED";
            break;
        case SQLITE_READONLY:
            codeInfo = "SQLITE_READONLY";
            break;
        case SQLITE_CANTOPEN:
            codeInfo = "SQLITE_CANTOPEN";
            break;
        case SQLITE_TOOBIG:
            codeInfo = "SQLITE_TOOBIG";
            break;
        case SQLITE_RANGE:
            codeInfo = "SQLITE_RANGE";
            break;
        case SQLITE_NOMEM:
            codeInfo = "SQLITE_NOMEM";
            break;
        case SQLITE_MISMATCH:
            codeInfo = "SQLITE_MISMATCH";
            break;
        case SQLITE_INTERRUPT:
            codeInfo = "SQLITE_INTERRUPT";
            break;
        default:
            codeInfo = "SQLiteError";
            break;
    }
    char buff[1024] ={0};
    snprintf(buff,sizeof(buff),"errMsg:%s code :%d code info:%s ",sqliteMsg,errCode,codeInfo);
    errMsg += buff;
}

int SqliteWrapper::collLocalized(void *not_used, int nKey1,//length of key1 
                    const void *pKey1, int nKey2, //length of key2
                    const void *pKey2){
    int rc, n;
    n = nKey1<nKey2 ? nKey1 : nKey2;
    rc = memcmp(pKey1, pKey2, n);
    if( rc==0 ){
        rc = nKey1 - nKey2;
    }
    return rc;
}

void SqliteWrapper::ErrorMsg(){
    ALOGD("error string %s ",sqlite3_errmsg(mDatabase));
}

