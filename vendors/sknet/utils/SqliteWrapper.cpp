#include"Log.h"
#include"SqliteWrapper.h"
#include"Mutex.h"
#include<stdio.h>

static const int SQLITE_SOFT_HEAP_LIMIT = 8 * 1024 * 1024;
static const int SQLITE_BUSY_TIMEOUT_MS = 2500;
static const int SQLITE_ENABLE_TRACE = 1;
static const int SQLITE_ENABLE_PROFILE = 1;

bool SqliteWrapper::mVerboseLog = false;

SqliteWrapper::SqliteWrapper(std::string path):mDatabasePath(path){
    mCanceled = 0;
    mDatabaseError = 0;
    mOpenFlags |= SQLITE_OPEN_READWRITE;
}

SqliteWrapper::~SqliteWrapper(){
    dispose();
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
    return statement;
}

void SqliteWrapper::getRowData(sqlite3_stmte *pStmt,int nCol){
    ColumnEntry *colEntries = new ColumnEntry[nCol]; 
    for (int i = 0; i < nCol; i++) {
        int type = sqlite3_column_type(pStmt, i);
        if (type == SQLITE_TEXT) {
            // TEXT data
            const char* text = reinterpret_cast<const char*>(
                    sqlite3_column_text(statement, i));
            colEntries[i].Value.charValue = text; 
            colEntries[i].Type = SQLITE_TEXT;
            colEntries[i].Length = sqlite3_column_bytes(statement, i) +1;
        } else if (type == SQLITE_INTEGER) {
            // INTEGER data
            int64_t value = sqlite3_column_int64(statement, i);
            colEntries[i].Value.longValue = value;
            colEntries[i].Type = SQLITE_INTEGER;
        } else if (type == SQLITE_FLOAT) {
            // FLOAT data
            double value = sqlite3_column_double(statement, i);
            colEntries[i].Value.floatValue = value;
            colEntries[i].Type = SQLITE_FLOAT;
        } else if (type == SQLITE_BLOB) {
            // BLOB data
            colEntries[i].Value.charValue = (const char *)sqlite3_column_blob(statement, i);
            colEntries[i].Length = sqlite3_column_bytes(statement, i);
            colEntries[i].Type = SQLITE_BLOB;
        } else if (type == SQLITE_NULL) {
            colEntries[i].Value.charValue = NULL; 
            colEntries[i].Type = SQLITE_NULL;
        } else {
            break;
        }
    }
}

int SqliteWrapper::execStmt(void *arg,sqlite3_stmt *pStmt,dCallback cb){
    int nCol = sqlite3_column_count(pStmt);
    std::string*colsName = NULL;
    while( 1 ){
        int i;
        rc = sqlite3_step(pStmt);
        if(rc == SQLITE_ROW){
            //get a row data and now get column name
            if(colsName != NULL && nCol > 0){
                colsName = new std::string[mCol];
                if(colsName == NULL){
                    ALOGW("execute statement fail  %p no memory ",pStmt);
                    return NO_MEMORY;
                }

                for(i=0; i<nCol; i++){
                    azCols[i] += (const char *)sqlite3_column_name(pStmt, i);
                    assert(!azCols[i].empty())
                }
            }

        }else if(rc == SQLITE_DONE){
            //sql execute done

        }

        /* Invoke the callback function if required */
        if( xCallback && (SQLITE_ROW==rc || 
                    (SQLITE_DONE==rc && !callbackIsInit
                     && db->flags&SQLITE_NullCallback)) ){
            if( !callbackIsInit ){
                azCols = sqlite3DbMallocZero(db, 2*nCol*sizeof(const char*) + 1);
                if( azCols==0 ){
                    goto exec_out;
                }
                for(i=0; i<nCol; i++){
                    azCols[i] = (char *)sqlite3_column_name(pStmt, i);
                    /* sqlite3VdbeSetColName() installs column names as UTF8
                     ** strings so there is no way for sqlite3_column_name() to fail. */
                    assert( azCols[i]!=0 );
                }
                callbackIsInit = 1;
            }
            if( rc==SQLITE_ROW ){
                azVals = &azCols[nCol];
                for(i=0; i<nCol; i++){
                    azVals[i] = (char *)sqlite3_column_text(pStmt, i);
                    if( !azVals[i] && sqlite3_column_type(pStmt, i)!=SQLITE_NULL ){
                        sqlite3OomFault(db);
                        goto exec_out;
                    }
                }
            }
            if( xCallback(pArg, nCol, azVals, azCols) ){
                /* EVIDENCE-OF: R-38229-40159 If the callback function to
                 ** sqlite3_exec() returns non-zero, then sqlite3_exec() will
                 ** return SQLITE_ABORT. */
                rc = SQLITE_ABORT;
                sqlite3VdbeFinalize((Vdbe *)pStmt);
                pStmt = 0;
                sqlite3Error(db, SQLITE_ABORT);
                goto exec_out;
            }
        }

        if( rc!=SQLITE_ROW ){
            rc = sqlite3VdbeFinalize((Vdbe *)pStmt);
            pStmt = 0;
            zSql = zLeftover;
            while( sqlite3Isspace(zSql[0]) ) zSql++;
            break;
        }
    }

    sqlite3DbFree(db, azCols);
    azCols = 0;
  }

exec_out:
  if( pStmt ) sqlite3VdbeFinalize((Vdbe *)pStmt);
  sqlite3DbFree(db, azCols);

  rc = sqlite3ApiExit(db, rc);
  if( rc!=SQLITE_OK && pzErrMsg ){
    int nErrMsg = 1 + sqlite3Strlen30(sqlite3_errmsg(db));
    *pzErrMsg = sqlite3Malloc(nErrMsg);
    if( *pzErrMsg ){
      memcpy(*pzErrMsg, sqlite3_errmsg(db), nErrMsg);
    }else{
      rc = SQLITE_NOMEM_BKPT;
      sqlite3Error(db, SQLITE_NOMEM);
    }
  }else if( pzErrMsg ){
    *pzErrMsg = 0;
  }

  assert( (rc&db->errMask)==rc );
  sqlite3_mutex_leave(db->mutex);

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
                if(cb(pStmt,pArg) < 0 || mCanceled){
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
    int rc = sqlite3_finalize(pStmt);
    if(rc != SQLITE_OK){
        ALOGE("sql fail %s error msg:%s",sql,sqlite3_errmsg(mDatabase));
        errorInc();
        return UNKNOWN_ERROR;
    }
    return OK;
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

